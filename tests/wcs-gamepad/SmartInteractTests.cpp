#include "SmartInteract.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace
{
    using namespace wcs_gamepad;

    SmartInteractCandidate Unit(uint64_t guid, float x, float y,
        SmartInteractPriority priority = SmartInteractPriority::GenericNpc)
    {
        SmartInteractCandidate value;
        value.guid = guid;
        value.name = "unit";
        value.kind = SmartInteractKind::Unit;
        value.priority = priority;
        value.position = {x, y, 0};
        value.interactable = true;
        return value;
    }

    SmartInteractCandidate GameObject(uint64_t guid, float x, float y)
    {
        SmartInteractCandidate value = Unit(guid, x, y, SmartInteractPriority::GameObject);
        value.kind = SmartInteractKind::GameObject;
        value.name = "gameobject";
        return value;
    }

    struct FakeWorld final : ISmartInteractWorld
    {
        std::optional<SmartInteractCandidate> current;
        SmartInteractVec3 player{};
        SmartInteractVec3 forward{1, 0, 0};
        bool combat{};
        bool targetSucceeds{true};
        bool interactSucceeds{true};
        std::vector<SmartInteractCandidate> candidates;
        std::vector<uint64_t> targets;
        std::vector<uint64_t> interactions;
        int debugCandidates{};
        std::vector<SmartInteractResult> results;

        std::optional<SmartInteractCandidate> CurrentTarget() override { return current; }
        SmartInteractVec3 PlayerPosition() override { return player; }
        SmartInteractVec3 ViewForward() override { return forward; }
        bool InCombat() override { return combat; }
        std::vector<SmartInteractCandidate> Candidates(float) override { return candidates; }
        bool Target(uint64_t guid) override { targets.push_back(guid); return targetSucceeds; }
        bool Interact(uint64_t guid) override { interactions.push_back(guid); return interactSucceeds; }
        void DebugCandidate(const SmartInteractCandidate&, const SmartInteractScore&, bool) override { ++debugCandidates; }
        void DebugResult(SmartInteractResult result) override { results.push_back(result); }
    };
}

int main()
{
    using namespace wcs_gamepad;
    constexpr float kRoot3 = 1.73205080757f;
    SmartInteractResolver resolver;

    SmartInteractConfig invalidConfig;
    invalidConfig.searchRadius = 0;
    invalidConfig.maximumAngleDegrees = std::numeric_limits<float>::quiet_NaN();
    invalidConfig.interactionRange = 99;
    invalidConfig.directionWeight = -5;
    const SmartInteractConfig sanitized = SanitizeSmartInteractConfig(invalidConfig);
    assert(sanitized.searchRadius == 1.0f && sanitized.maximumAngleDegrees == 60.0f);
    assert(sanitized.interactionRange == 1.0f && sanitized.directionWeight == 0.0f);

    const auto cameraForward = SmartInteractViewForward({3, 4, 9}, 0.0f);
    assert(std::abs(cameraForward.x - .6f) < .001f && std::abs(cameraForward.y - .8f) < .001f);
    const auto facingFallback = SmartInteractViewForward({std::numeric_limits<float>::quiet_NaN(), 0, 0}, 1.57079632679f);
    assert(std::abs(facingFallback.x) < .001f && std::abs(facingFallback.y - 1.0f) < .001f);

    // Cone boundaries are inclusive, while candidates just beyond 60 degrees are rejected.
    assert(resolver.Score(Unit(1, 1, kRoot3), {}, {1, 0, 0}));
    assert(!resolver.Score(Unit(2, 1, 1.74f), {}, {1, 0, 0}));
    assert(!resolver.Score(Unit(3, 13, 0), {}, {1, 0, 0}));

    // Camera alignment can outweigh proximity, and distance still breaks equally aligned choices.
    auto best = resolver.FindBest({Unit(1, 8, 0), Unit(2, 2, 2)}, {}, {1, 0, 0});
    assert(best && best->candidate.guid == 1);
    best = resolver.FindBest({Unit(1, 8, 0), Unit(2, 2, 0)}, {}, {1, 0, 0});
    assert(best && best->candidate.guid == 2);

    // Equal scores are stable regardless of enumeration order.
    best = resolver.FindBest({Unit(9, 4, 0), Unit(4, 4, 0)}, {}, {1, 0, 0});
    assert(best && best->candidate.guid == 4);

    SmartInteractCandidate unsupported = Unit(20, 2, 0);
    unsupported.interactable = false;
    assert(!resolver.Score(unsupported, {}, {1, 0, 0}));
    unsupported.interactable = true; unsupported.active = false;
    assert(!resolver.Score(unsupported, {}, {1, 0, 0}));
    unsupported.active = true; unsupported.visible = false;
    assert(!resolver.Score(unsupported, {}, {1, 0, 0}));
    unsupported.visible = true; unsupported.selectable = false;
    assert(!resolver.Score(unsupported, {}, {1, 0, 0}));

    // Verified quest and service state gets the configured priority bonuses.
    const auto generic = resolver.Score(Unit(1, 4, 0), {}, {1, 0, 0});
    const auto service = resolver.Score(Unit(2, 4, 0, SmartInteractPriority::ServiceNpc), {}, {1, 0, 0});
    const auto available = resolver.Score(Unit(3, 4, 0, SmartInteractPriority::QuestAvailable), {}, {1, 0, 0});
    const auto turnIn = resolver.Score(Unit(4, 4, 0, SmartInteractPriority::QuestTurnIn), {}, {1, 0, 0});
    assert(generic && service && available && turnIn);
    assert(std::abs(service->total - generic->total - 10.0f) < .001f);
    assert(std::abs(available->total - generic->total - 15.0f) < .001f);
    assert(std::abs(turnIn->total - generic->total - 20.0f) < .001f);

    FakeWorld world;
    SmartInteractExecutor executor(world);

    world.current = Unit(10, 3, 0);
    assert(executor.Execute() == SmartInteractResult::Success);
    assert(world.interactions == std::vector<uint64_t>{10} && world.targets.empty());

    world = {}; world.current = Unit(11, 7, 0);
    SmartInteractExecutor outOfRange(world);
    assert(outOfRange.Execute() == SmartInteractResult::TargetOutOfRange);
    assert(world.targets.empty() && world.interactions.empty());

    world = {}; world.candidates = {Unit(12, 4, 0)};
    SmartInteractExecutor aligned(world);
    assert(aligned.Execute() == SmartInteractResult::Success);
    assert(world.targets == std::vector<uint64_t>{12});
    assert(world.interactions == std::vector<uint64_t>{12});

    // A target selected by Smart Interact is replaceable on a later camera-directed press.
    world.current = Unit(12, 4, 2);
    world.candidates = {Unit(12, 4, 2), Unit(21, 3, 0)};
    world.targets.clear(); world.interactions.clear();
    assert(aligned.Execute() == SmartInteractResult::Success);
    assert(world.targets == std::vector<uint64_t>{21});
    assert(world.interactions == std::vector<uint64_t>{21});

    world = {};
    SmartInteractExecutor none(world);
    assert(none.Execute() == SmartInteractResult::NoTarget);

    // A non-interactable target may be replaced out of combat, but remains authoritative in combat.
    world = {}; world.current = unsupported; world.candidates = {Unit(13, 3, 0)};
    SmartInteractExecutor replacement(world);
    assert(replacement.Execute() == SmartInteractResult::Success);
    assert(world.targets == std::vector<uint64_t>{13});
    world = {}; world.current = unsupported; world.combat = true; world.candidates = {Unit(14, 3, 0)};
    SmartInteractExecutor blocked(world);
    assert(blocked.Execute() == SmartInteractResult::BlockedInCombat);
    assert(world.targets.empty() && world.interactions.empty());

    world = {}; world.candidates = {Unit(15, 8, 0)};
    SmartInteractExecutor selected(world);
    assert(selected.Execute() == SmartInteractResult::TargetSelected);
    assert(world.targets == std::vector<uint64_t>{15} && world.interactions.empty());

    world = {}; world.candidates = {GameObject(16, 3, 0)};
    SmartInteractExecutor usableObject(world);
    assert(usableObject.Execute() == SmartInteractResult::Success);
    assert(world.targets.empty() && world.interactions == std::vector<uint64_t>{16});
    world = {}; world.candidates = {GameObject(17, 8, 0)};
    SmartInteractExecutor distantObject(world);
    assert(distantObject.Execute() == SmartInteractResult::TargetOutOfRange);
    assert(world.targets.empty() && world.interactions.empty());

    world = {}; world.candidates = {Unit(18, 3, 0)}; world.targetSucceeds = false;
    SmartInteractExecutor invalidTarget(world);
    assert(invalidTarget.Execute() == SmartInteractResult::InvalidTarget);
    assert(world.interactions.empty());
    world = {}; world.candidates = {GameObject(19, 3, 0)}; world.interactSucceeds = false;
    SmartInteractExecutor failedInteraction(world);
    assert(failedInteraction.Execute() == SmartInteractResult::InvalidTarget);
    assert(world.targets.empty());

    SmartInteractConfig debugConfig; debugConfig.debug = true;
    world = {}; world.candidates = {Unit(20, 3, 0)};
    SmartInteractExecutor debug(world, debugConfig);
    assert(debug.Execute() == SmartInteractResult::Success);
    assert(world.debugCandidates == 1 && world.results == std::vector<SmartInteractResult>{SmartInteractResult::Success});

    std::cout << "smart interact tests passed\n";
    return 0;
}
