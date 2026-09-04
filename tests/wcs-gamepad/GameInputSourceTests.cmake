if(NOT DEFINED GAME_INPUT_SOURCE)
    message(FATAL_ERROR "GAME_INPUT_SOURCE is required")
endif()

file(READ "${GAME_INPUT_SOURCE}" source_text)
string(FIND "${source_text}" "case GameCommand::ToggleWorldMap: wxl::game::script::Execute(\"ToggleFrame(WorldMapFrame)\")" map_command)
if(map_command EQUAL -1)
    message(FATAL_ERROR "ToggleWorldMap must use the WoW 3.3.5a TOGGLEWORLDMAP binding implementation")
endif()

string(FIND "${source_text}" "script::Execute(\"ToggleWorldMap()\")" unsupported_map_call)
if(NOT unsupported_map_call EQUAL -1)
    message(FATAL_ERROR "ToggleWorldMap() is not the WoW 3.3.5a TOGGLEWORLDMAP binding implementation")
endif()
