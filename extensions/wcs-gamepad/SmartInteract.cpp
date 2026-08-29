#include "SmartInteract.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace wcs_gamepad
{
    namespace
    {
        constexpr float kPi = 3.14159265358979323846f;
        float Length2(float x, float y) { return std::sqrt(x*x + y*y); }
        float Distance(SmartInteractVec3 a, SmartInteractVec3 b)
        {
            const float x = a.x-b.x, y = a.y-b.y, z = a.z-b.z;
            return std::sqrt(x*x + y*y + z*z);
        }
    }

    const char* SmartInteractResultName(SmartInteractResult result)
    {
        switch (result)
        {
        case SmartInteractResult::Success: return "SUCCESS";
        case SmartInteractResult::TargetSelected: return "TARGET_SELECTED";
        case SmartInteractResult::TargetOutOfRange: return "TARGET_OUT_OF_RANGE";
        case SmartInteractResult::NoTarget: return "NO_TARGET";
        case SmartInteractResult::BlockedInCombat: return "BLOCKED_IN_COMBAT";
        default: return "INVALID_TARGET";
        }
    }

    SmartInteractVec3 SmartInteractViewForward(SmartInteractVec3 cameraForward, float playerFacing)
    {
        const float length = Length2(cameraForward.x, cameraForward.y);
        if (std::isfinite(length) && length >= .0001f)
            return {cameraForward.x / length, cameraForward.y / length, 0.0f};
        if (std::isfinite(playerFacing))
            return {std::cos(playerFacing), std::sin(playerFacing), 0.0f};
        return {};
    }

    SmartInteractConfig SanitizeSmartInteractConfig(SmartInteractConfig config)
    {
        const SmartInteractConfig defaults;
        auto finite = [](float value, float fallback) { return std::isfinite(value) ? value : fallback; };
        config.searchRadius = std::clamp(finite(config.searchRadius, defaults.searchRadius), 1.0f, 50.0f);
        config.maximumAngleDegrees = std::clamp(finite(config.maximumAngleDegrees, defaults.maximumAngleDegrees), 1.0f, 89.0f);
        config.interactionRange = std::clamp(finite(config.interactionRange, defaults.interactionRange), 1.0f, config.searchRadius);
        config.directionWeight = std::clamp(finite(config.directionWeight, defaults.directionWeight), 0.0f, 1000.0f);
        config.distanceWeight = std::clamp(finite(config.distanceWeight, defaults.distanceWeight), 0.0f, 1000.0f);
        config.questTurnInBonus = std::clamp(finite(config.questTurnInBonus, defaults.questTurnInBonus), 0.0f, 1000.0f);
        config.questAvailableBonus = std::clamp(finite(config.questAvailableBonus, defaults.questAvailableBonus), 0.0f, 1000.0f);
        config.serviceNpcBonus = std::clamp(finite(config.serviceNpcBonus, defaults.serviceNpcBonus), 0.0f, 1000.0f);
        config.genericNpcBonus = std::clamp(finite(config.genericNpcBonus, defaults.genericNpcBonus), 0.0f, 1000.0f);
        config.gameObjectBonus = std::clamp(finite(config.gameObjectBonus, defaults.gameObjectBonus), 0.0f, 1000.0f);
        return config;
    }

    float SmartInteractResolver::TypeBonus(SmartInteractPriority priority) const
    {
        switch (priority)
        {
        case SmartInteractPriority::QuestTurnIn: return config_.questTurnInBonus;
        case SmartInteractPriority::QuestAvailable: return config_.questAvailableBonus;
        case SmartInteractPriority::ServiceNpc: return config_.serviceNpcBonus;
        case SmartInteractPriority::GameObject: return config_.gameObjectBonus;
        default: return config_.genericNpcBonus;
        }
    }

    std::optional<SmartInteractScore> SmartInteractResolver::Score(const SmartInteractCandidate& candidate, SmartInteractVec3 player, SmartInteractVec3 forward) const
    {
        if (!candidate.guid || !candidate.active || !candidate.visible || !candidate.interactable || !candidate.selectable) return std::nullopt;
        const float distance = Distance(player, candidate.position);
        if (!std::isfinite(distance) || distance > config_.searchRadius) return std::nullopt;
        const float fx = forward.x, fy = forward.y, fl = Length2(fx, fy);
        const float dx = candidate.position.x-player.x, dy = candidate.position.y-player.y, dl = Length2(dx, dy);
        if (!std::isfinite(fl) || fl < .0001f || !std::isfinite(dl)) return std::nullopt;
        float angle = 0.0f;
        if (dl >= .0001f)
        {
            const float dot = std::clamp((fx*dx + fy*dy) / (fl*dl), -1.0f, 1.0f);
            angle = std::acos(dot) * 180.0f / kPi;
        }
        if (angle > config_.maximumAngleDegrees) return std::nullopt;
        SmartInteractScore score;
        score.distance = distance;
        score.angleDegrees = angle;
        score.direction = config_.directionWeight * (1.0f - angle/config_.maximumAngleDegrees);
        score.proximity = config_.distanceWeight * (1.0f - distance/config_.searchRadius);
        score.typeBonus = TypeBonus(candidate.priority);
        score.total = score.direction + score.proximity + score.typeBonus;
        return score;
    }

    std::optional<SmartInteractSelection> SmartInteractResolver::FindBest(const std::vector<SmartInteractCandidate>& candidates, SmartInteractVec3 player, SmartInteractVec3 forward) const
    {
        std::optional<SmartInteractSelection> best;
        for (const auto& candidate : candidates)
        {
            const auto score = Score(candidate, player, forward);
            if (!score) continue;
            if (!best || score->total > best->score.total + .0001f ||
                (std::abs(score->total-best->score.total) <= .0001f &&
                 (score->distance < best->score.distance-.0001f ||
                  (std::abs(score->distance-best->score.distance) <= .0001f && candidate.guid < best->candidate.guid))))
                best = SmartInteractSelection{candidate, *score};
        }
        return best;
    }

    bool SmartInteractExecutor::InRange(const SmartInteractCandidate& candidate, SmartInteractVec3 player) const
    { return Distance(candidate.position, player) <= config_.interactionRange; }

    SmartInteractResult SmartInteractExecutor::Finish(SmartInteractResult result)
    { if (config_.debug) world_.DebugResult(result); return result; }

    SmartInteractResult SmartInteractExecutor::Execute()
    {
        const SmartInteractVec3 player = world_.PlayerPosition();
        const auto current = world_.CurrentTarget();
        const bool currentEligible = current && current->interactable && current->active && current->visible && current->selectable;
        const bool managedCurrent = currentEligible && current->guid == managedTarget_;
        if (!current || current->guid != managedTarget_) managedTarget_ = 0;

        // Targets chosen outside Smart Interact remain authoritative. A target Smart Interact chose
        // itself may be replaced on a later press when the camera clearly indicates another NPC.
        if (currentEligible && (!managedCurrent || world_.InCombat()))
        {
            if (!InRange(*current, player)) return Finish(SmartInteractResult::TargetOutOfRange);
            return Finish(world_.Interact(current->guid) ? SmartInteractResult::Success : SmartInteractResult::InvalidTarget);
        }
        if (world_.InCombat()) return Finish(SmartInteractResult::BlockedInCombat);

        const auto candidates = world_.Candidates(config_.searchRadius);
        const SmartInteractVec3 forward = world_.ViewForward();
        const auto selected = resolver_.FindBest(candidates, player, forward);
        if (config_.debug)
            for (const auto& candidate : candidates)
                if (const auto score = resolver_.Score(candidate, player, forward))
                    world_.DebugCandidate(candidate, *score, selected && selected->candidate.guid == candidate.guid);
        if (!selected)
        {
            if (!managedCurrent) return Finish(SmartInteractResult::NoTarget);
            if (!InRange(*current, player)) return Finish(SmartInteractResult::TargetOutOfRange);
            return Finish(world_.Interact(current->guid) ? SmartInteractResult::Success : SmartInteractResult::InvalidTarget);
        }

        if (selected->candidate.kind == SmartInteractKind::Unit)
        {
            if ((!current || current->guid != selected->candidate.guid) && !world_.Target(selected->candidate.guid))
                return Finish(SmartInteractResult::InvalidTarget);
            managedTarget_ = selected->candidate.guid;
        }
        if (!InRange(selected->candidate, player))
        {
            if (current && current->guid == selected->candidate.guid) return Finish(SmartInteractResult::TargetOutOfRange);
            return Finish(selected->candidate.kind == SmartInteractKind::Unit ? SmartInteractResult::TargetSelected : SmartInteractResult::TargetOutOfRange);
        }
        return Finish(world_.Interact(selected->candidate.guid) ? SmartInteractResult::Success : SmartInteractResult::InvalidTarget);
    }
}
