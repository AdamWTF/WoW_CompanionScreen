#pragma once

#include "Protocol.hpp"

#include <array>
#include <functional>
#include <string>

namespace wxl_thor
{
    class InputDispatcher
    {
    public:
        using ErrorSink = std::function<void(std::string)>;
        explicit InputDispatcher(ErrorSink errors) : errors_(std::move(errors)) {}
        void Dispatch(const Command& command);
        void ReleaseAll();

    private:
        static void* WowWindow();
        static unsigned VirtualKey(const std::string& key);
        void Key(unsigned key, bool down);
        void Modifiers(uint8_t modifiers, bool down);
        bool PointerReady(void*& window);
        void PointerButton(int button, bool down, bool force = false);

        ErrorSink errors_;
        std::array<bool, 256> keys_{};
        std::array<bool, 3> buttons_{};
    };
}
