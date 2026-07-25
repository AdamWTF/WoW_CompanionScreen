// Height-based terrain layer blending (per-layer height maps + tiling exponent) for split map tiles.
// Copyright (C) 2026 WarcraftXL
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

// The extended terrain shaders are installed files, loaded with no involvement from us. This file
// owns the other half: the per-chunk DATA those shaders read.
//
// Which height map belongs to which layer, and the tiling exponent that goes with it, are
// properties of the tile currently resident under the camera. They change per chunk, so they can
// only be published at the draw: the four height maps go to stages s9..s12 and the c22..c25 block
// carries their scale/offset plus the per-layer blend shape. A sibling hook on the per-chunk
// vertex-constant build divides the layer UV tiling by the same tile's exponent, which the height
// samples inherit for free because they reuse each layer's own texture coordinates.
//
// SEAM RULE -- the one thing this file must not get wrong. Every terrain draw runs the extended
// shader, not just the ones with height data, so what a layer contributes has to be decided by its
// TEXTURE and nothing else. Two neighbouring surfaces that share a texture must publish the same
// numbers for it, or the difference shows up as a straight line along the boundary between them.
// That is why the height pair and the sharpen strength are both per layer, and why a layer that
// resolves no height map is left at neutral instead of the whole surface falling back: a fallback
// decided per surface is a fallback that draws its own outline.
//
// Cost when there is nothing to do: one settings load, the resident-split-tile count, and the map's
// height-texturing flag -- a map without height data pays three loads per chunk and draws stock.

#include "config.hpp"
#include "engine/hook/Hook.hpp"
#include "engine/hook/Registry.hpp"
#include "engine/events/Event.hpp"
#include "client/CMapArea/AdtSplit.hpp"
#include "client/CMapChunk/HeightBlend.hpp"

#include "common/Config.hpp"
#include "common/Log.hpp"
#include "offsets/engine/Gx.hpp"
#include "offsets/engine/Shader.hpp"
#include "offsets/game/ADT.hpp"

#include <atomic>
#include <cstdint>
#include <cstdlib>

namespace
{
    namespace adt   = wxl::offsets::game::adt;
    namespace shoff = wxl::offsets::engine::shader;
    namespace gxoff = wxl::offsets::engine::gx;
    namespace ev    = wxl::events;
    namespace split = wxl::runtime::adtsplit;
    namespace hb    = wxl::features::heightblend;

    template <class T>
    inline T& At(void* base, size_t off) { return *reinterpret_cast<T*>(static_cast<uint8_t*>(base) + off); }

    /// Layer counts the blend is defined for; a single layer has nothing to blend.
    constexpr uint32_t kMinLayers = 2;
    constexpr uint32_t kMaxLayers = 4;

    /**
     * @brief The c22..c25 block the extended shaders read, in upload order.
     *
     * Every field is neutral at ZERO: zero scale and zero bias make the height factor exactly 1,
     * zero sharpness makes the sharpen term exactly 1, and zero renormalize strength leaves the
     * weights untouched. So an all-zero block reproduces the plain weighted sum exactly, and the
     * blend is inert until something asks for it -- per layer, not per surface.
     */
    struct Constants
    {
        float scale[4];     ///< c22: per-layer height scale
        float bias[4];      ///< c23: per-layer (height offset - 1)
        float sharpness[4]; ///< c24: per-layer sharpen strength
        float shape[4];     ///< c25: .x renormalize strength
    };
    constexpr int kConstantVec4s = 4;
    static_assert(sizeof(Constants) == kConstantVec4s * 4 * sizeof(float),
                  "c22..c25 upload as one contiguous block");

    hb::Settings g_settings;
    bool         g_installed = false;

    adt::Map_SurfaceChunkDrawShaderFn g_origDraw           = nullptr;
    adt::Map_BuildTerrainConstantsFn  g_origBuildConstants = nullptr;

    std::atomic<uint32_t> g_statChunks{ 0 };
    std::atomic<bool>     g_activeFlag{ false }; // last fast-path verdict, for the status readout

    // Whether the constant block currently holds the neutral state. The extended shaders run on
    // every terrain draw, so the resting state between the chunks that do have height data has to
    // be the inert one, and a draw that never reaches the publish below must find it already there.
    bool g_neutralStanding = false;

    void UploadConstants(const Constants& c)
    {
        reinterpret_cast<shoff::ShaderConstantsSetHelperFn>(shoff::kShaderConstantsSet)(
            4, static_cast<int>(adt::kPsConstTerrainBindBase), &c.scale[0], kConstantVec4s);
    }

    /// Puts the blend back in its inert state, so a terrain draw with no height data of its own
    /// reproduces the plain weighted sum.
    void PublishNeutral()
    {
        const Constants neutral{};
        UploadConstants(neutral);
        g_neutralStanding = true;
    }

    void OnDeviceReset(void*, const void*)
    {
        // Constants do not survive a device reset, so the next draw has to publish them again.
        g_neutralStanding = false;
    }

    // ---------------------------------------------------------------- the draw-leaf detour
    /// Fast-path gate: runtime toggle + any split tile resident + the map's height-texturing flag.
    inline bool Active()
    {
        if (!g_settings.enabled) return false;
        if (split::ResidentTilesRelaxed() == 0) return false;
        if ((*reinterpret_cast<const uint32_t*>(adt::kMphdFlags) & 0x80u) == 0) return false;
        return true;
    }

    /**
     * @brief Detours the per-chunk terrain draw on the programmable path.
     *
     * Publishes one chunk's height data: its resolved height maps on their stages, and the c22..c25
     * block describing them. Layers that resolve nothing keep the neutral entries they were built
     * with, so they contribute exactly their plain coverage; the block is returned to neutral
     * afterwards so the next chunk starts from the inert state.
     */
    void __fastcall hkSurfaceChunkDrawShader(void* node, void* edx)
    {
        // Every terrain draw runs the extended shader, so the inert state has to be standing before
        // any chunk without height data of its own reaches the draw.
        if (!g_neutralStanding) PublishNeutral();

        const bool active = Active();
        g_activeFlag.store(active, std::memory_order_relaxed);
        if (!active) { g_origDraw(node, edx); return; }

        const uint32_t n     = At<uint8_t>(node, adt::kOffChunkNodeLayerCount);
        const uint16_t flags = At<uint16_t>(node, adt::kOffChunkNodeFlags);
        void* mapChunk       = At<void*>(node, adt::kOffChunkNodeChunk);
        if (n < kMinLayers || n > kMaxLayers || (flags & 0x4u) != 0 || !mapChunk)
        {
            g_origDraw(node, edx); // one layer, or a reflected layer: nothing to blend
            return;
        }

        // chunk -> owning tile, through the same link the split fill seam walks
        void* area = nullptr;
        const uint32_t link = At<uint32_t>(mapChunk, adt::kOffChunkTexOwnerSrc);
        if (link != 0 && (link & 1u) == 0)
            area = *reinterpret_cast<void**>(static_cast<uintptr_t>(link) + 8);
        if (!area) { g_origDraw(node, edx); return; }

        void* gxDev = *reinterpret_cast<void**>(gxoff::kGxDevicePtr);
        if (!gxDev) { g_origDraw(node, edx); return; }

        auto rsSet      = reinterpret_cast<adt::Map_SamplerBindFn>(adt::kSetSamplerTexture);
        auto texResolve = reinterpret_cast<adt::Map_TexResolveFn>(adt::kTexResolve);

        // Per LAYER, never per chunk: each layer that resolves a height map gets its own pair and
        // its own sharpen strength, each layer that does not keeps the neutral zeros and so
        // contributes exactly the coverage it always did. A single unresolved layer must not change
        // how the others are shaded, or the surfaces around it stop agreeing on shared textures.
        Constants c{};
        uint32_t resolved = 0;
        for (uint32_t i = 0; i < n; ++i)
        {
            const uint32_t texId = At<uint32_t>(node, adt::kOffChunkLayerRecords +
                                                          i * adt::kChunkLayerRecordStride +
                                                          adt::kOffLayerSlotTexId);
            split::HeightLayer hl{};
            if (!split::GetHeightLayer(area, texId, hl)) break; // the tile carries no height data
            void* gxTex = hl.texture ? texResolve(hl.texture, 0, 0) : nullptr;
            if (!gxTex) continue;
            c.scale[i]     = hl.heightScale;
            c.bias[i]      = hl.heightOffset - 1.0f;
            c.sharpness[i] = g_settings.sharpness;
            rsSet(gxDev, nullptr, adt::kSamplerSelHeight0 + i, gxTex);
            ++resolved;
        }
        if (resolved == 0) { g_origDraw(node, edx); return; } // nothing to publish, stay inert

        // Renormalizing rebalances every layer against the others, so it only makes sense once some
        // layer actually carries height -- otherwise it would rescale plain coverage for no reason.
        c.shape[0] = 1.0f;
        UploadConstants(c);
        g_neutralStanding = false;

        g_origDraw(node, edx); // untouched original: vertex shader pick, layer binds, one draw call

        for (uint32_t i = 0; i < n; ++i)
            rsSet(gxDev, nullptr, adt::kSamplerSelHeight0 + i, nullptr);
        PublishNeutral();

        g_statChunks.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Post-hook on the per-chunk vertex-constant build: applies each layer's UV tiling
     *        exponent by dividing its tiling vec4 .xy by (1 << exponent) and re-uploading the range.
     *
     * The height samples reuse the layer texture coordinates, so they inherit the corrected tiling
     * without a second constant. The original always runs first and rebuilds the whole block per
     * chunk, so the divide can never accumulate. The exponent comes from the texture, so every
     * surface drawing that texture scales it the same way.
     */
    void __cdecl hkBuildTerrainConstants(void* node, uint32_t a1, uint32_t a2)
    {
        g_origBuildConstants(node, a1, a2);
        if (!Active()) return;

        const uint32_t n = At<uint8_t>(node, adt::kOffChunkNodeLayerCount);
        void* mapChunk   = At<void*>(node, adt::kOffChunkNodeChunk);
        if (n == 0 || n > kMaxLayers || !mapChunk) return;
        const uint32_t link = At<uint32_t>(mapChunk, adt::kOffChunkTexOwnerSrc);
        if (link == 0 || (link & 1u) != 0) return;
        void* area = *reinterpret_cast<void**>(static_cast<uintptr_t>(link) + 8);
        if (!area) return;

        float* c18 = reinterpret_cast<float*>(adt::kVsConstC18);
        bool any = false;
        for (uint32_t i = 0; i < n; ++i)
        {
            const uint32_t texId = At<uint32_t>(node, adt::kOffChunkLayerRecords +
                                                          i * adt::kChunkLayerRecordStride +
                                                          adt::kOffLayerSlotTexId);
            split::HeightLayer hl{};
            if (!split::GetHeightLayer(area, texId, hl)) return; // no height data: leave it alone
            if (hl.tilingExp == 0) continue;
            const float div = static_cast<float>(1u << (hl.tilingExp & 0xFu));
            c18[i * 4 + 0] /= div;
            c18[i * 4 + 1] /= div;
            any = true;
        }
        if (any)
            reinterpret_cast<shoff::ShaderConstantsSetHelperFn>(shoff::kShaderConstantsSet)(
                0, static_cast<int>(adt::kVsConstC18Reg), c18, static_cast<int>(n));
    }

    // ---------------------------------------------------------------- install
    bool InstallHeightBlend()
    {
        char buf[64];
        if (wxl::config::Raw("WXL_ADT_HEIGHT_BLEND", buf, sizeof buf))
            g_settings.enabled = wxl::config::Truthy(buf, true);
        if (wxl::config::Raw("WXL_ADT_HEIGHT_SHARPNESS", buf, sizeof buf))
        {
            const float v = static_cast<float>(std::atof(buf));
            if (v >= 0.0f && v <= 16.0f) g_settings.sharpness = v;
        }
        // Which channel of a height map is read is decided when the shaders are generated, not here.

        ev::Subscribe(ev::Event::OnDeviceReset, &OnDeviceReset, nullptr);

        g_installed = wxl::hook::Install("AdtHeightBlend.SurfaceChunkDrawShader",
                                         adt::kSurfaceChunkDrawShader,
                                         &hkSurfaceChunkDrawShader, &g_origDraw);
        const bool uv = wxl::hook::Install("AdtHeightBlend.BuildTerrainConstants",
                                           adt::kBuildTerrainConstants,
                                           &hkBuildTerrainConstants, &g_origBuildConstants);
        if (!uv)
            WLOG_WARN("height-blend: UV-tiling constants hook failed; tiling exponents ignored");
        if (g_installed)
            WLOG_INFO("height-blend: terrain data detour installed (enabled=%d sharpness=%.2f)",
                      g_settings.enabled ? 1 : 0, g_settings.sharpness);
        else
            WLOG_WARN("height-blend: data detour install failed; terrain renders stock");
        return true; // non-fatal
    }
}

namespace wxl::features::heightblend
{
    Settings& Get() { return g_settings; }

    Stats GetStats()
    {
        Stats s{};
        s.chunksDrawn = g_statChunks.load(std::memory_order_relaxed);
        s.active      = g_activeFlag.load(std::memory_order_relaxed);
        return s;
    }

    bool Installed() { return g_installed; }
}

WXL_REGISTER_FEATURE("adt-height-blend", wxl::features::modernADTSupport, InstallHeightBlend)
