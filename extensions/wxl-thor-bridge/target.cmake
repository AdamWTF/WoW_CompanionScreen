target_link_libraries(${wxl_ext_name} PRIVATE ws2_32 bcrypt crypt32 user32)

if(BUILD_TESTING)
    add_executable(wxl-thor-bridge-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wxl-thor-bridge/ProtocolTests.cpp"
        "${wxl_ext_dir}/Json.cpp"
        "${wxl_ext_dir}/Protocol.cpp")
    target_include_directories(wxl-thor-bridge-tests PRIVATE "${wxl_ext_dir}")
    target_compile_definitions(wxl-thor-bridge-tests PRIVATE ${WXL_DEFS})
    add_test(NAME wxl-thor-bridge-protocol COMMAND wxl-thor-bridge-tests)

    add_executable(wxl-thor-bridge-websocket-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wxl-thor-bridge/WebSocketTests.cpp"
        "${wxl_ext_dir}/Json.cpp"
        "${wxl_ext_dir}/Protocol.cpp"
        "${wxl_ext_dir}/Pairing.cpp"
        "${wxl_ext_dir}/WebSocketServer.cpp")
    target_include_directories(wxl-thor-bridge-websocket-tests PRIVATE "${wxl_ext_dir}")
    target_compile_definitions(wxl-thor-bridge-websocket-tests PRIVATE ${WXL_DEFS})
    target_link_libraries(wxl-thor-bridge-websocket-tests PRIVATE ws2_32 bcrypt crypt32)
    add_test(NAME wxl-thor-bridge-websocket COMMAND wxl-thor-bridge-websocket-tests)
endif()
