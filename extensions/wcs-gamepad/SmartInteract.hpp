#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace wcs_gamepad
{
    struct SmartInteractVec3 { float x{}, y{}, z{}; };
    enum class SmartInteractKind { Unit, GameObject };
    enum class SmartInteractPriority { GenericNpc, GameObject, ServiceNpc, QuestAvailable, QuestTurnIn, SkinnableCorpse, LootableCorpse };
    enum class SmartInteractResult { Success, TargetSelected, TargetOutOfRange, NoTarget, BlockedInCombat, InvalidTarget };

    const char* SmartInteractResultName(SmartInteractResult result);
    SmartInteractVec3 SmartInteractViewForward(SmartInteractVec3 cameraForward, float playerFacing);

    struct SmartInteractConfig
    {
        float searchRadius{12.0f};
        float maximumAngleDegrees{60.0f};
        float interactionRange{5.0f};
        float directionWeight{100.0f};
        float distanceWeight{50.0f};
        float questTurnInBonus{40.0f};
        float questAvailableBonus{35.0f};
        float serviceNpcBonus{30.0f};
        float genericNpcBonus{20.0f};
        float gameObjectBonus{15.0f};
        float skinnableCorpseBonus{45.0f};
        float lootableCorpseBonus{50.0f};
        bool debug{};
    };

    SmartInteractConfig SanitizeSmartInteractConfig(SmartInteractConfig config);

    struct SmartInteractCandidate
    {
        uint64_t guid{};
        std::string name;
        SmartInteractKind kind{SmartInteractKind::Unit};
        SmartInteractPriority priority{SmartInteractPriority::GenericNpc};
        SmartInteractVec3 position{};
        bool active{true}, visible{true}, interactable{}, selectable{true};
    };

    struct SmartInteractScore
    {
        float distance{}, angleDegrees{}, direction{}, proximity{}, typeBonus{}, total{};
    };

    struct SmartInteractSelection
    {
        SmartInteractCandidate candidate;
        SmartInteractScore score;
    };

    class ISmartInteractWorld
    {
    public:
        virtual ~ISmartInteractWorld() = default;
        virtual std::optional<SmartInteractCandidate> CurrentTarget() = 0;
        virtual SmartInteractVec3 PlayerPosition() = 0;
        virtual SmartInteractVec3 ViewForward() = 0;
        virtual bool InCombat() = 0;
        virtual std::vector<SmartInteractCandidate> Candidates(float radius) = 0;
        virtual bool Target(uint64_t guid) = 0;
        virtual bool Interact(uint64_t guid) = 0;
        virtual void DebugCandidate(const SmartInteractCandidate&, const SmartInteractScore&, bool) {}
        virtual void DebugResult(SmartInteractResult) {}
    };

    class SmartInteractResolver final
    {
    public:
        explicit SmartInteractResolver(SmartInteractConfig config = {}) : config_(SanitizeSmartInteractConfig(config)) {}
        std::optional<SmartInteractScore> Score(const SmartInteractCandidate& candidate, SmartInteractVec3 player, SmartInteractVec3 forward) const;
        std::optional<SmartInteractSelection> FindBest(const std::vector<SmartInteractCandidate>& candidates, SmartInteractVec3 player, SmartInteractVec3 forward) const;
    private:
        float TypeBonus(SmartInteractPriority priority) const;
        SmartInteractConfig config_;
    };

    class SmartInteractExecutor final
    {
    public:
        SmartInteractExecutor(ISmartInteractWorld& world, SmartInteractConfig config = {}) : world_(world), config_(SanitizeSmartInteractConfig(config)), resolver_(config_) {}
        SmartInteractResult Execute();
    private:
        bool InRange(const SmartInteractCandidate& candidate, SmartInteractVec3 player) const;
        SmartInteractResult Finish(SmartInteractResult result);
        ISmartInteractWorld& world_;
        SmartInteractConfig config_;
        SmartInteractResolver resolver_;
        uint64_t managedTarget_{};
    };
}
