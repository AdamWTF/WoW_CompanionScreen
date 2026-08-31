#include "NativeSmartInteractWorld.hpp"
#include "ExtensionApi.hpp"

#include "game/Camera.hpp"
#include "game/Interaction.hpp"
#include "game/Unit.hpp"
#include "game/World.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace wcs_gamepad
{
    namespace
    {
        constexpr uint32_t kNpcGossip = 0x00000001;
        constexpr uint32_t kNpcServiceMask = 0x01FFFFFC;

        SmartInteractVec3 Position(void* object)
        {
            float value[3]{}; wxl::game::world::Position(object, value);
            return {value[0], value[1], value[2]};
        }
    }

    void* NativeSmartInteractWorld::Player() const
    {
        return wxl::game::world::ResolveObject(wxl::game::world::ActivePlayerGuid(), wxl::game::world::kTypeMaskPlayer);
    }

    std::optional<SmartInteractCandidate> NativeSmartInteractWorld::Describe(uint64_t guid, void* object, void* player) const
    {
        if (!guid || !object || !player || guid == wxl::game::world::ActivePlayerGuid()) return std::nullopt;
        const unsigned type = wxl::game::world::TypeMask(object);
        SmartInteractCandidate candidate;
        candidate.guid = guid;
        candidate.position = Position(object);
        candidate.active = std::isfinite(candidate.position.x) && std::isfinite(candidate.position.y) && std::isfinite(candidate.position.z);
        candidate.visible = candidate.active;
        candidate.name = wxl::game::interaction::Name(object);

        if ((type & wxl::game::world::kTypeMaskUnit) != 0 && (type & wxl::game::world::kTypeMaskPlayer) == 0)
        {
            const uint32_t npcFlags = wxl::game::interaction::UnitNpcFlags(object);
            const uint32_t unitFlags = wxl::game::interaction::UnitFlags(object);
            const uint32_t dynamicFlags = wxl::game::interaction::UnitDynamicFlags(object);
            const bool friendlyOrNeutral = wxl::game::unit::Reaction(player, object) >= 3;
            const bool lootable = (dynamicFlags & wxl::game::interaction::off::kUnitDynamicFlagLootable) != 0;
            const bool skinnable = (unitFlags & wxl::game::interaction::off::kUnitFlagSkinnable) != 0;
            candidate.kind = SmartInteractKind::Unit;
            candidate.selectable = candidate.active && wxl::game::interaction::Selectable(object);
            candidate.interactable = lootable || skinnable || (friendlyOrNeutral && npcFlags != 0);
            // NPC_FLAG_QUESTGIVER establishes eligibility, but not verified available/turn-in state.
            if (lootable) candidate.priority = SmartInteractPriority::LootableCorpse;
            else if (skinnable) candidate.priority = SmartInteractPriority::SkinnableCorpse;
            else if (npcFlags & (kNpcGossip | kNpcServiceMask)) candidate.priority = SmartInteractPriority::ServiceNpc;
            else candidate.priority = SmartInteractPriority::GenericNpc;
            return candidate;
        }
        if ((type & wxl::game::world::kTypeMaskGameObject) != 0)
        {
            candidate.kind = SmartInteractKind::GameObject;
            candidate.priority = SmartInteractPriority::GameObject;
            candidate.interactable = wxl::game::interaction::GameObjectInteractable(object);
            // GameObjects such as mailboxes are directly interactable but are not unit targets, so
            // the unit selection virtual is neither required nor consulted for this classification.
            candidate.selectable = candidate.active && candidate.interactable;
            return candidate;
        }
        return std::nullopt;
    }

    std::optional<SmartInteractCandidate> NativeSmartInteractWorld::CurrentTarget()
    {
        const uint64_t guid = wxl::game::world::TargetGuid();
        if (!guid) return std::nullopt;
        void* object = wxl::game::world::ResolveObject(guid, wxl::game::world::kTypeMaskUnit | wxl::game::world::kTypeMaskPlayer);
        return Describe(guid, object, Player());
    }

    SmartInteractVec3 NativeSmartInteractWorld::PlayerPosition()
    { return Position(Player()); }

    SmartInteractVec3 NativeSmartInteractWorld::ViewForward()
    {
        const float* view = wxl::game::camera::GetView();
        const SmartInteractVec3 camera{view[2], view[6], view[10]};
        void* const player = Player();
        const float facing = player ? wxl::game::interaction::Facing(player) : std::numeric_limits<float>::quiet_NaN();
        return SmartInteractViewForward(camera, facing);
    }

    bool NativeSmartInteractWorld::InCombat()
    { return wxl::game::interaction::InCombat(Player()); }

    std::vector<SmartInteractCandidate> NativeSmartInteractWorld::Candidates(float)
    {
        std::vector<SmartInteractCandidate> result;
        void* const player = Player();
        if (!player) return result;
        wxl::game::world::ForEachObject(wxl::game::world::kTypeMaskUnit | wxl::game::world::kTypeMaskGameObject,
            [&](uint64_t guid, void* object)
            {
                if (auto candidate = Describe(guid, object, player); candidate && candidate->interactable)
                    result.push_back(std::move(*candidate));
                return true;
            });
        return result;
    }

    bool NativeSmartInteractWorld::Target(uint64_t guid)
    {
        if (!wxl::game::world::ResolveObject(guid, wxl::game::world::kTypeMaskUnit)) return false;
        wxl::game::interaction::Target(guid);
        return wxl::game::world::TargetGuid() == guid;
    }

    bool NativeSmartInteractWorld::Interact(uint64_t guid)
    {
        if (!wxl::game::world::ResolveObject(guid, wxl::game::world::kTypeMaskObject | wxl::game::world::kTypeMaskUnit | wxl::game::world::kTypeMaskGameObject)) return false;
        return wxl::game::interaction::Interact(guid);
    }

    void NativeSmartInteractWorld::DebugCandidate(const SmartInteractCandidate& candidate, const SmartInteractScore& score, bool selected)
    {
        if (!g_api) return;
        g_api->Log(WXL_LOG_INFO, kTag, "[SmartInteract] candidate=%s guid=%08X%08X type=%s distance=%.2f angle=%.1f direction=%.1f proximity=%.1f typeBonus=%.1f score=%.1f%s",
            candidate.name.empty() ? "<unnamed>" : candidate.name.c_str(), uint32_t(candidate.guid >> 32), uint32_t(candidate.guid),
            candidate.kind == SmartInteractKind::Unit ? "NPC" : "GameObject", score.distance, score.angleDegrees,
            score.direction, score.proximity, score.typeBonus, score.total, selected ? " SELECTED" : "");
    }

    void NativeSmartInteractWorld::DebugResult(SmartInteractResult result)
    { if (g_api) g_api->Log(WXL_LOG_INFO, kTag, "[SmartInteract] result=%s", SmartInteractResultName(result)); }
}
