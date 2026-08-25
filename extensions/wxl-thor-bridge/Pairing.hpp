#pragma once

#include "Json.hpp"

#include <mutex>
#include <string>
#include <utility>

namespace wxl_thor
{
    class PairingManager
    {
    public:
        explicit PairingManager(std::string path = "Extensions\\wxl-thor-bridge\\pairing.dat") : path_(std::move(path)) {}
        bool Initialise(bool required);
        bool Required() const { return required_; }
        bool IsPaired() const;
        std::string PairingCode() const;
        std::string DeviceName() const;
        bool Authenticate(const std::string& token);
        bool Pair(const std::string& code, std::string deviceId, std::string deviceName, std::string& token);
        void Forget();

    private:
        static std::string RandomToken(size_t bytes);
        static std::string RandomCode();
        bool Load();
        bool Save() const;
        void FailedAttempt();

        mutable std::mutex mutex_;
        bool required_ = true;
        std::string code_;
        std::string token_;
        std::string deviceId_;
        std::string deviceName_;
        unsigned failures_ = 0;
        unsigned long long blockedUntil_ = 0;
        std::string path_;
    };
}
