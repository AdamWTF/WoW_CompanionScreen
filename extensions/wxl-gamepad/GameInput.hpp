#pragma once

#include "ControllerConfig.hpp"
#include "ThorPadActions.hpp"

#include <cstdint>
#include <memory>

namespace wxl_gamepad
{
    enum class MovementControl { Forward, Backward, StrafeLeft, StrafeRight };
    enum class GameCommand { ToggleGameMenu, ToggleAllBags, NextView };
    enum class UINavigationCommand { Up, Down, Left, Right, Confirm, Back };

    class IGameInput
    {
    public:
        virtual ~IGameInput() = default;
        virtual bool Foreground() const = 0;
        virtual void WoWAction(int slot) = 0;
        virtual void SystemAction(ThorPadSystemAction action, InputState state, uint32_t time) = 0;
        virtual void Movement(MovementControl control, bool down, uint32_t time) = 0;
        virtual void Target(const KeyChord& chord) = 0;
        virtual void Command(GameCommand command) = 0;
        virtual void Camera(bool active, float dx, float dy) = 0;
        virtual void PointerMove(int dx, int dy) = 0;
        virtual void PointerClick(bool right) = 0;
        virtual void MovePointerNormalized(float x, float y) = 0;
        virtual void UINavigation(UINavigationCommand command) = 0;
        virtual void ReleaseAll(uint32_t time) = 0;
    };

    class GameInput final : public IGameInput
    {
    public:
        explicit GameInput(const ControllerConfig& config);
        ~GameInput();
        bool Foreground() const override;
        void WoWAction(int slot) override;
        void SystemAction(ThorPadSystemAction action, InputState state, uint32_t time) override;
        void Movement(MovementControl control, bool down, uint32_t time) override;
        void Target(const KeyChord& chord) override;
        void Command(GameCommand command) override;
        void Camera(bool active, float dx, float dy) override;
        void PointerMove(int dx, int dy) override;
        void PointerClick(bool right) override;
        void MovePointerNormalized(float x, float y) override;
        void UINavigation(UINavigationCommand command) override;
        void ReleaseAll(uint32_t time) override;
        void FlushPointerActions();
        const char* LastSmartInteractResult() const;
    private:
        struct SmartInteractState;
        static void* Window(); static intptr_t KeyParameter(unsigned key, bool down); void Key(unsigned key, bool down);
        void MouseButton(bool right, bool down, bool force = false); void Move(float dx, float dy, bool camera);
        bool movement_[4]{}, keys_[256]{}, rightMouse_{}; float cameraRemainderX_{}, cameraRemainderY_{};
        bool pendingPointerMove_{}, pendingPointerClick_{}, pendingPointerRight_{}; float pendingPointerX_{}, pendingPointerY_{};
        std::unique_ptr<SmartInteractState> smartInteract_;
    };
}
