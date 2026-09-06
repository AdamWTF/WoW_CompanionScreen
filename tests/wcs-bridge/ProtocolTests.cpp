#include "Json.hpp"
#include "Protocol.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    void Check(bool condition, const char* message)
    {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
    }

    wcs_bridge::json::Value Parse(const char* source)
    {
        wcs_bridge::json::Value value; std::string error; Check(wcs_bridge::json::Parse(source, value, error), error.c_str()); return value;
    }
}

int main()
{
    using namespace wcs_bridge;
    using namespace wcs_bridge::json;

    Value unicode = Parse(R"({"text":"Thor \u2603 \ud83d\udee1"})");
    Check(ValidUtf8(*unicode.Find("text")->String()), "unicode JSON must produce valid UTF-8");
    Value rejected; std::string error; Check(!Parse(R"({"a":1,})", rejected, error), "trailing comma must fail");

    Command command; std::string code;
    Check(ParseCommand(Parse(R"({"type":"key.press","key":"B","modifiers":["SHIFT"]})"), command, code), "key.press must parse");
    Check(command.kind == CommandKind::KeyPress && command.modifiers == 1, "key.press fields");
    Check(ParseCommand(Parse(R"({"type":"pointer.move","dx":12,"dy":-4})"), command, code), "pointer.move must parse");
    Check(command.x == 12 && command.y == -4, "pointer delta");
    Check(!ParseCommand(Parse(R"({"type":"action.press","slot":0})"), command, code) && code == "invalid-action-slot", "slot zero must fail");
    Check(ParseCommand(Parse(R"({"type":"action.press","slot":24})"), command, code) && command.value + 24 == 48, "Thor 24 maps to native 48");
    Check(ParseCommand(Parse(R"({"type":"party.select","member":4})"), command, code) && command.kind == CommandKind::PartySelect && command.value == 4, "party member command");
    Check(!ParseCommand(Parse(R"({"type":"party.select","member":5})"), command, code) && code == "invalid-party-member", "party member bounds");

    StateStore state;
    Value snapshot = state.SnapshotMessage();
    const auto* data = snapshot.Find("data"); const auto* actions = data->Find("actions");
    Check(actions->Find("slots")->ArrayValue()->size() == 24, "default snapshot has 24 slots");
    Check(data->Find("party")->Find("members")->ArrayValue()->empty(), "default snapshot has empty party");
    Check(state.PublishSnapshot(Parse(R"({"player":{"name":"Adfox","level":37},"actions":{"slots":[{"slot":1,"empty":false},{"slot":24,"empty":true}]},"party":{"members":[{"slot":2,"guid":"0x00000000000000A2","name":"Second"},{"slot":1,"guid":"0x00000000000000A1","name":"First"}]}})"), error), "snapshot accepted");
    snapshot = state.SnapshotMessage();
    Check(*snapshot.Find("data")->Find("game")->Find("state")->String() == "world", "addon snapshot marks lifecycle in-world");
    const auto* slots = snapshot.Find("data")->Find("actions")->Find("slots")->ArrayValue();
    Check(slots->size() == 24, "normalized snapshot has 24 slots");
    int64_t slot = 0; Check((*slots)[0].Find("slot")->Integer(slot) && slot == 1, "first slot fixed");
    Check((*slots)[1].Find("empty") != nullptr, "missing slot filled empty");
    const auto* members = snapshot.Find("data")->Find("party")->Find("members")->ArrayValue();
    Check(members->size() == 2, "party members retained");
    Check(state.PartyGuid(1).value_or(0) == 0xA1 && state.PartyGuid(2).value_or(0) == 0xA2, "party members normalized and resolved");
    Check(!state.PartyGuid(3).has_value(), "missing party slot is unavailable");
    Check(state.PublishEvent("party.state", Parse(R"({"members":[{"slot":4,"guid":"0x00000000000000A4"}]})"), error), "party update accepted");
    Check(state.PartyGuid(4).value_or(0) == 0xA4 && !state.PartyGuid(1).has_value(), "party update replaces roster");
    Check(!state.PublishEvent("party.state", Parse(R"({"members":[{"slot":1,"guid":"bad"}]})"), error), "malformed party GUID rejected");
    Check(!state.PublishEvent("party.state", Parse(R"({"members":[{"slot":1,"guid":"0x0000000000000001"},{"slot":1,"guid":"0x0000000000000002"}]})"), error), "duplicate party slot rejected");
    Check(!state.PublishEvent("party.state", Parse(R"({"members":[{"slot":1,"guid":"0x0000000000000001"},{"slot":2,"guid":"0x0000000000000002"},{"slot":3,"guid":"0x0000000000000003"},{"slot":4,"guid":"0x0000000000000004"},{"slot":4,"guid":"0x0000000000000005"}]})"), error), "oversized party rejected");
    Check(state.PublishEvent("player.money", Parse(R"({"copper":124874218})"), error), "money update accepted");
    Check(state.PublishEvent("action.updated", Parse(R"({"slot":12,"empty":false,"count":2})"), error), "action update accepted");
    state.SetGameState("loading", true); snapshot = state.SnapshotMessage();
    Check(snapshot.Find("data")->Find("player")->IsNull(), "world clear drops player state");
    Check(snapshot.Find("data")->Find("actions")->Find("slots")->ArrayValue()->size() == 24, "world clear preserves layout");
    Check(snapshot.Find("data")->Find("party")->Find("members")->ArrayValue()->empty(), "world clear drops party state");
    Check(!state.PartyGuid(4).has_value(), "party targeting unavailable outside world");

    Check(ValidUtf8("hello"), "ASCII UTF-8");
    const std::string invalid("\xc0\x80", 2); Check(!ValidUtf8(invalid), "overlong UTF-8 rejected");
    std::cout << "WoW Companion Screen bridge protocol tests passed\n";
    return 0;
}
