// Native modern-M2 reader: drives the stock per-field offset->pointer readers over the modern body.
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

#include "client/CM2Shared/M2NativeInternal.hpp"

#include "game/Binding.hpp"
#include "offsets/game/M2.hpp"

#include <cstdint>

namespace off = wxl::offsets::game::m2;
namespace fmt = wxl::structure::m2;

namespace wxl::runtime::m2native::detail
{
    /**
     * @brief Drives the stock per-field offset->pointer readers over the modern body, in the stock parse
     *        order (RE'd from CM2Shared__FinishLoading). Every reader used here covers a record type
     *        whose stride is identical between 264 and 272-274; cameras and particles (modern-wider
     *        strides) were parked count=0 by the caller and are skipped.
     * @return true when every reader accepted its array (bounds-valid offsets).
     */
    bool DriveStockWalk(uint8_t* base, uint32_t size, fmt::M2Header* h)
    {
        auto rd = [&](uintptr_t fn, void* arr) -> bool {
            return wxl::game::Native<off::M2_HeaderReadFn>(fn)(base, size, h, arr) != 0;
        };

        if (!rd(off::kReadByteArray,    &h->name)) return false;
        if (!rd(off::kReadInt32Array,   &h->globalLoops)) return false;
        if (!rd(off::kReadAnimations,   &h->sequences)) return false;
        if (!rd(off::kReadInt16Array,   &h->sequenceLookup)) return false;
        if (!rd(off::kReadBones,        &h->bones)) return false; // reads the fixed-up sequences
        if (!rd(off::kReadInt16Array,   &h->boneLookup)) return false;
        if (!rd(off::kReadVertices,     &h->vertices)) return false;
        if (!rd(off::kReadColors,       &h->colors)) return false;
        if (!rd(off::kReadTextures,     &h->textures)) return false;
        if (!rd(off::kReadTransparency, &h->textureWeights)) return false;
        if (!rd(off::kReadUVAnimation,  &h->textureTransforms)) return false;
        if (!rd(off::kReadInt16Array,   &h->textureReplacements)) return false;
        if (!rd(off::kReadInt32Array,   &h->materials)) return false;
        if (!rd(off::kReadInt16Array,   &h->boneCombos)) return false;
        if (!rd(off::kReadInt16Array,   &h->textureCombos)) return false;
        if (!rd(off::kReadInt16Array,   &h->textureUnitLookup)) return false;
        if (!rd(off::kReadInt16Array,   &h->textureWeightCombos)) return false;
        if (!rd(off::kReadInt16Array,   &h->textureTransformCombos)) return false;
        if (!rd(off::kReadInt16Array,   &h->collisionIndices)) return false;
        if (!rd(off::kReadVector3,      &h->collisionPositions)) return false;
        if (!rd(off::kReadVector3,      &h->collisionNormals)) return false;
        if (!rd(off::kReadAttachments,  &h->attachments)) return false;
        if (!rd(off::kReadInt16Array,   &h->attachmentLookup)) return false;
        if (!rd(off::kReadEvents,       &h->events)) return false;
        if (!rd(off::kReadLights,       &h->lights)) return false;
        // cameras + cameraLookup parked (count 0) by the caller; the stock 264-stride camera reader must
        // never touch a 0x74-stride modern record.
        if (!rd(off::kRibbonDeRelocate, &h->ribbonEmitters)) return false; // stride 0xB0, identical
        // particleEmitters: the caller left them populated (count != 0) only when the client can step the
        // modern 0x1EC stride -- ParticleStride.cpp patched the reader's stride sites. A parked array is
        // count 0 and the stock reader is a no-op on it, so this is safe either way.
        if (h->particleEmitters.count)
            if (!rd(off::kReadParticleEmitters, &h->particleEmitters)) return false;
        // The two modern emitter encodings the stock client can't read (packed multi-texture textureId,
        // blend mode 7) are handled by TEACHING THE CLIENT to read them -- ParticleStride.cpp patches the
        // read sites and the blend table. The record itself is left exactly as the file has it.
        if (h->globalFlags & fmt::kFlagUseTextureCombinerCombos)
            if (!rd(off::kReadInt16Array, &h->textureCombinerCombos)) return false;
        return true;
    }
}
