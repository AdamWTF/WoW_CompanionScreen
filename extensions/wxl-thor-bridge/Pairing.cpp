#include "Pairing.hpp"

#include <windows.h>
#include <bcrypt.h>
#include <dpapi.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

namespace wxl_thor
{
    namespace
    {
        constexpr char kCodeAlphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
        constexpr char kBase64Url[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

        bool ConstantEqual(const std::string& left, const std::string& right)
        {
            unsigned difference = unsigned(left.size() ^ right.size()); const size_t count = (std::max)(left.size(), right.size());
            for (size_t i = 0; i < count; ++i) difference |= unsigned((i < left.size() ? left[i] : 0) ^ (i < right.size() ? right[i] : 0));
            return difference == 0;
        }

        bool ReadAll(const std::string& path, std::vector<unsigned char>& bytes)
        {
            FILE* file = nullptr; if (fopen_s(&file, path.c_str(), "rb") != 0 || !file) return false;
            fseek(file, 0, SEEK_END); const long size = ftell(file); fseek(file, 0, SEEK_SET);
            if (size <= 0 || size > 64 * 1024) { fclose(file); return false; }
            bytes.resize(size_t(size)); const bool ok = fread(bytes.data(), 1, bytes.size(), file) == bytes.size(); fclose(file); return ok;
        }
    }

    std::string PairingManager::RandomToken(size_t bytes)
    {
        std::vector<unsigned char> random(bytes);
        if (BCryptGenRandom(nullptr, random.data(), ULONG(random.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) return {};
        std::string result; result.reserve((bytes * 4 + 2) / 3);
        for (size_t i = 0; i < random.size(); i += 3)
        {
            const uint32_t value = uint32_t(random[i]) << 16 | (i + 1 < random.size() ? uint32_t(random[i + 1]) << 8 : 0) | (i + 2 < random.size() ? random[i + 2] : 0);
            result.push_back(kBase64Url[(value >> 18) & 63]); result.push_back(kBase64Url[(value >> 12) & 63]);
            if (i + 1 < random.size()) result.push_back(kBase64Url[(value >> 6) & 63]);
            if (i + 2 < random.size()) result.push_back(kBase64Url[value & 63]);
        }
        return result;
    }

    std::string PairingManager::RandomCode()
    {
        std::array<unsigned char, 8> bytes{};
        if (BCryptGenRandom(nullptr, bytes.data(), ULONG(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) return "UNAVAILABLE";
        std::string code; code.reserve(9);
        for (size_t i = 0; i < bytes.size(); ++i) { if (i == 4) code.push_back('-'); code.push_back(kCodeAlphabet[bytes[i] % (sizeof(kCodeAlphabet) - 1)]); }
        return code;
    }

    bool PairingManager::Initialise(bool required)
    {
        required_ = required;
        if (required_ && !Load()) code_ = RandomCode();
        return !required_ || !code_.empty() || !token_.empty();
    }

    bool PairingManager::Load()
    {
        std::vector<unsigned char> encrypted; if (!ReadAll(path_, encrypted)) return false;
        DATA_BLOB input{DWORD(encrypted.size()), encrypted.data()}, output{};
        if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) return false;
        std::string plain(reinterpret_cast<char*>(output.pbData), output.cbData); LocalFree(output.pbData);
        json::Value root; std::string error;
        if (!json::Parse(plain, root, error)) return false;
        const auto* version = root.Find("version"); int64_t v = 0;
        const auto* token = root.Find("token"); const auto* id = root.Find("deviceId"); const auto* name = root.Find("deviceName");
        if (!version || !version->Integer(v) || v != 1 || !token || !token->String() || !id || !id->String() || !name || !name->String()) return false;
        token_ = *token->String(); deviceId_ = *id->String(); deviceName_ = *name->String(); return !token_.empty();
    }

    bool PairingManager::Save() const
    {
        const std::string plain = json::Dump(json::Value::Object{{"version", 1}, {"token", token_}, {"deviceId", deviceId_}, {"deviceName", deviceName_}});
        DATA_BLOB input{DWORD(plain.size()), reinterpret_cast<BYTE*>(const_cast<char*>(plain.data()))}, output{};
        if (!CryptProtectData(&input, L"Thor Bridge pairing", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) return false;
        const std::string temp = path_ + ".tmp"; FILE* file = nullptr;
        bool ok = fopen_s(&file, temp.c_str(), "wb") == 0 && file;
        if (ok) { ok = fwrite(output.pbData, 1, output.cbData, file) == output.cbData && fflush(file) == 0; fclose(file); }
        LocalFree(output.pbData);
        if (!ok) { DeleteFileA(temp.c_str()); return false; }
        if (!MoveFileExA(temp.c_str(), path_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { DeleteFileA(temp.c_str()); return false; }
        return true;
    }

    bool PairingManager::IsPaired() const { std::lock_guard lock(mutex_); return !token_.empty(); }
    std::string PairingManager::PairingCode() const { std::lock_guard lock(mutex_); return token_.empty() ? code_ : std::string{}; }
    std::string PairingManager::DeviceName() const { std::lock_guard lock(mutex_); return deviceName_; }

    void PairingManager::FailedAttempt()
    {
        if (++failures_ >= 5) { failures_ = 0; code_ = RandomCode(); blockedUntil_ = GetTickCount64() + 5000; }
    }

    bool PairingManager::Authenticate(const std::string& token)
    {
        std::lock_guard lock(mutex_); if (!required_) return true;
        if (GetTickCount64() < blockedUntil_ || token_.empty() || !ConstantEqual(token_, token)) { FailedAttempt(); return false; }
        failures_ = 0; return true;
    }

    bool PairingManager::Pair(const std::string& code, std::string deviceId, std::string deviceName, std::string& token)
    {
        std::lock_guard lock(mutex_);
        if (!required_) { token.clear(); return true; }
        if (GetTickCount64() < blockedUntil_ || !token_.empty() || !ConstantEqual(code_, code)) { FailedAttempt(); return false; }
        if (deviceId.empty() || deviceId.size() > 128 || deviceName.empty() || deviceName.size() > 128) { FailedAttempt(); return false; }
        token_ = RandomToken(32); if (token_.empty()) return false;
        deviceId_ = std::move(deviceId); deviceName_ = std::move(deviceName);
        if (!Save()) { token_.clear(); deviceId_.clear(); deviceName_.clear(); return false; }
        token = token_; code_.clear(); failures_ = 0; return true;
    }

    void PairingManager::Forget()
    {
        std::lock_guard lock(mutex_); token_.clear(); deviceId_.clear(); deviceName_.clear(); failures_ = 0; blockedUntil_ = 0; code_ = RandomCode(); DeleteFileA(path_.c_str());
    }
}
