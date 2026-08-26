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

    wxl_thor::json::Value Parse(const char* source)
    {
        wxl_thor::json::Value value; std::string error; Check(wxl_thor::json::Parse(source, value, error), error.c_str()); return value;
    }
}

int main()
{
    using namespace wxl_thor;
    using namespace wxl_thor::json;

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

    StateStore state;
    Value snapshot = state.SnapshotMessage();
    const auto* data = snapshot.Find("data"); const auto* actions = data->Find("actions");
    Check(actions->Find("slots")->ArrayValue()->size() == 24, "default snapshot has 24 slots");
    Check(state.PublishSnapshot(Parse(R"({"player":{"name":"Adfox","level":37},"actions":{"slots":[{"slot":1,"empty":false},{"slot":24,"empty":true}]}})"), error), "snapshot accepted");
    snapshot = state.SnapshotMessage();
    Check(*snapshot.Find("data")->Find("game")->Find("state")->String() == "world", "addon snapshot marks lifecycle in-world");
    const auto* slots = snapshot.Find("data")->Find("actions")->Find("slots")->ArrayValue();
    Check(slots->size() == 24, "normalized snapshot has 24 slots");
    int64_t slot = 0; Check((*slots)[0].Find("slot")->Integer(slot) && slot == 1, "first slot fixed");
    Check((*slots)[1].Find("empty") != nullptr, "missing slot filled empty");
    Check(state.PublishEvent("player.money", Parse(R"({"copper":124874218})"), error), "money update accepted");
    Check(state.PublishEvent("action.updated", Parse(R"({"slot":12,"empty":false,"count":2})"), error), "action update accepted");
    state.SetGameState("loading", true); snapshot = state.SnapshotMessage();
    Check(snapshot.Find("data")->Find("player")->IsNull(), "world clear drops player state");
    Check(snapshot.Find("data")->Find("actions")->Find("slots")->ArrayValue()->size() == 24, "world clear preserves layout");

    Check(ValidUtf8("hello"), "ASCII UTF-8");
    const std::string invalid("\xc0\x80", 2); Check(!ValidUtf8(invalid), "overlong UTF-8 rejected");
    std::cout << "Thor Bridge protocol tests passed\n";
    return 0;
}
