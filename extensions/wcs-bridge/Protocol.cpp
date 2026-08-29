#include "Protocol.hpp"

#include <algorithm>
#include <cmath>
#include <set>

namespace wcs_bridge
{
    namespace
    {
        const std::string* Type(const json::Value& root)
        {
            const auto* value = root.Find("type");
            return value ? value->String() : nullptr;
        }

        bool ReadInt(const json::Value& root, const char* name, int minimum, int maximum, int& out)
        {
            const auto* value = root.Find(name); int64_t number = 0;
            if (!value || !value->Integer(number) || number < minimum || number > maximum) return false;
            out = int(number); return true;
        }

        bool Modifiers(const json::Value& root, uint8_t& flags)
        {
            flags = 0; const auto* value = root.Find("modifiers");
            if (!value) return true;
            const auto* array = value->ArrayValue(); if (!array || array->size() > 3) return false;
            for (const auto& item : *array)
            {
                const auto* name = item.String(); if (!name) return false;
                uint8_t bit = *name == "SHIFT" ? 1 : *name == "CTRL" ? 2 : *name == "ALT" ? 4 : 0;
                if (!bit || (flags & bit)) return false; flags |= bit;
            }
            return true;
        }

        bool PointerButton(const json::Value& root, int& button)
        {
            const auto* value = root.Find("button"); const auto* name = value ? value->String() : nullptr;
            if (!name) return false;
            if (*name == "left") button = 0;
            else if (*name == "right") button = 1;
            else if (*name == "middle") button = 2;
            else return false;
            return true;
        }
    }

    bool ValidUtf8(std::string_view value)
    {
        for (size_t i = 0; i < value.size();)
        {
            const uint8_t c = uint8_t(value[i++]); if (c <= 0x7f) continue;
            unsigned count = 0; uint32_t cp = 0;
            if ((c & 0xe0) == 0xc0) { count = 1; cp = c & 0x1f; if (cp < 2) return false; }
            else if ((c & 0xf0) == 0xe0) { count = 2; cp = c & 0x0f; }
            else if ((c & 0xf8) == 0xf0) { count = 3; cp = c & 7; }
            else return false;
            if (i + count > value.size()) return false;
            for (unsigned n = 0; n < count; ++n) { const uint8_t d = uint8_t(value[i++]); if ((d & 0xc0) != 0x80) return false; cp = (cp << 6) | (d & 0x3f); }
            if ((count == 2 && cp < 0x800) || (count == 3 && cp < 0x10000) || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) return false;
        }
        return true;
    }

    bool ParseCommand(const json::Value& root, Command& command, std::string& errorCode)
    {
        if (!root.IsObject()) { errorCode = "invalid-message"; return false; }
        const auto* type = Type(root); if (!type) { errorCode = "invalid-message"; return false; }
        if (*type == "key.press" || *type == "key.down" || *type == "key.up")
        {
            const auto* keyValue = root.Find("key"); const auto* key = keyValue ? keyValue->String() : nullptr;
            if (!key || key->empty() || key->size() > 16 || !Modifiers(root, command.modifiers)) { errorCode = "invalid-message"; return false; }
            command.kind = *type == "key.press" ? CommandKind::KeyPress : *type == "key.down" ? CommandKind::KeyDown : CommandKind::KeyUp;
            command.key = *key; return true;
        }
        if (*type == "text.insert")
        {
            const auto* value = root.Find("text"); const auto* text = value ? value->String() : nullptr;
            if (!text || text->size() > 4096 || !ValidUtf8(*text)) { errorCode = "invalid-message"; return false; }
            command.kind = CommandKind::TextInsert; command.text = *text; return true;
        }
        if (*type == "pointer.move")
        {
            if (!ReadInt(root, "dx", -32768, 32767, command.x) || !ReadInt(root, "dy", -32768, 32767, command.y)) { errorCode = "invalid-message"; return false; }
            command.kind = CommandKind::PointerMove; return true;
        }
        if (*type == "pointer.scroll")
        {
            if (!ReadInt(root, "delta", -120, 120, command.value) || command.value == 0) { errorCode = "invalid-message"; return false; }
            command.kind = CommandKind::PointerScroll; return true;
        }
        if (*type == "pointer.click" || *type == "pointer.down" || *type == "pointer.up")
        {
            if (!PointerButton(root, command.value)) { errorCode = "invalid-message"; return false; }
            command.kind = *type == "pointer.click" ? CommandKind::PointerClick : *type == "pointer.down" ? CommandKind::PointerDown : CommandKind::PointerUp;
            return true;
        }
        if (*type == "action.press")
        {
            if (!ReadInt(root, "slot", 1, 24, command.value)) { errorCode = "invalid-action-slot"; return false; }
            command.kind = CommandKind::ActionPress; return true;
        }
        errorCode = "unsupported-command"; return false;
    }

    json::Value ErrorMessage(std::string code, std::string detail)
    {
        json::Value::Object result{{"type", "error"}, {"code", std::move(code)}};
        if (!detail.empty()) result.emplace("message", std::move(detail));
        return result;
    }

    StateStore::StateStore() : actions_(EmptyActions()) {}

    json::Value StateStore::EmptyActions()
    {
        json::Value::Array slots; slots.reserve(24);
        for (int slot = 1; slot <= 24; ++slot) slots.emplace_back(json::Value::Object{{"slot", slot}, {"empty", true}});
        return json::Value(json::Value::Object{{"slots", std::move(slots)}});
    }

    bool StateStore::NormalizeActions(json::Value& actions, std::string& error)
    {
        const auto* object = actions.ObjectValue(); const auto* slotsValue = object ? actions.Find("slots") : nullptr;
        const auto* slots = slotsValue ? slotsValue->ArrayValue() : nullptr;
        if (!slots) { error = "actions.slots must be an array"; return false; }
        std::array<json::Value, 24> normalized;
        for (int i = 0; i < 24; ++i) normalized[size_t(i)] = json::Value::Object{{"slot", i + 1}, {"empty", true}};
        std::array<bool, 24> seen{};
        for (const auto& entry : *slots)
        {
            int64_t slot = 0; const auto* slotValue = entry.Find("slot");
            if (!entry.IsObject() || !slotValue || !slotValue->Integer(slot) || slot < 1 || slot > 24 || seen[size_t(slot - 1)])
            { error = "action slot must be unique and in 1..24"; return false; }
            seen[size_t(slot - 1)] = true; normalized[size_t(slot - 1)] = entry;
        }
        json::Value::Array result; result.reserve(24); for (auto& entry : normalized) result.push_back(std::move(entry));
        actions = json::Value::Object{{"slots", std::move(result)}}; return true;
    }

    bool StateStore::PublishSnapshot(const json::Value& data, std::string& error)
    {
        if (!data.IsObject()) { error = "snapshot data must be an object"; return false; }
        json::Value player = data.Find("player") ? *data.Find("player") : json::Value(nullptr);
        json::Value actions = data.Find("actions") ? *data.Find("actions") : EmptyActions();
        if (!player.IsNull() && !player.IsObject()) { error = "player must be an object or null"; return false; }
        if (!NormalizeActions(actions, error)) return false;
        // Addon snapshots can only be published from the in-world FrameScript context. Treat one as
        // authoritative lifecycle evidence as well as authoritative player/action state. The client's
        // CWorldEnter routine does not return until the world is left, so the lower-level lifecycle
        // detour cannot reliably mark the active session as "world" on its own.
        std::lock_guard lock(mutex_); gameState_ = "world"; player_ = std::move(player); actions_ = std::move(actions); return true;
    }

    bool StateStore::PublishEvent(std::string_view type, const json::Value& data, std::string& error)
    {
        static const std::set<std::string_view> playerEvents = {"player.state", "player.money", "player.experience", "player.bags"};
        if (!data.IsObject()) { error = "event data must be an object"; return false; }
        std::lock_guard lock(mutex_);
        if (playerEvents.contains(type))
        {
            if (!player_.IsObject()) player_ = json::Value::Object{};
            auto* player = player_.ObjectValue();
            if (type == "player.state") for (const auto& [key, value] : *data.ObjectValue()) player->insert_or_assign(key, value);
            else if (type == "player.money") { const auto* copper = data.Find("copper"); if (!copper || !copper->IsNumber()) { error = "money requires copper"; return false; } player->insert_or_assign("money", *copper); }
            else if (type == "player.experience") player->insert_or_assign("experience", data);
            else player->insert_or_assign("bags", data);
            return true;
        }
        if (type == "actions.state")
        {
            json::Value copy = data; if (!NormalizeActions(copy, error)) return false; actions_ = std::move(copy); return true;
        }
        if (type == "action.updated")
        {
            int64_t slot = 0; const auto* slotValue = data.Find("slot");
            if (!slotValue || !slotValue->Integer(slot) || slot < 1 || slot > 24) { error = "invalid action slot"; return false; }
            auto* slots = actions_.ObjectValue()->at("slots").ArrayValue(); (*slots)[size_t(slot - 1)] = data; return true;
        }
        error = "event type is not publishable"; return false;
    }

    void StateStore::SetGameState(std::string state, bool clearWorldState)
    {
        std::lock_guard lock(mutex_); gameState_ = std::move(state);
        if (clearWorldState) { player_ = nullptr; actions_ = EmptyActions(); }
    }

    json::Value StateStore::SnapshotMessage() const
    {
        std::lock_guard lock(mutex_);
        json::Value::Object data{{"game", json::Value::Object{{"state", gameState_}}}, {"player", player_}, {"actions", actions_}};
        return json::Value::Object{{"type", "state.snapshot"}, {"data", std::move(data)}};
    }

    std::string StateStore::GameState() const { std::lock_guard lock(mutex_); return gameState_; }
}
