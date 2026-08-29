target_link_libraries(${wxl_ext_name} PRIVATE ws2_32 bcrypt crypt32 user32)

if(BUILD_TESTING)
    add_executable(wcs-bridge-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wcs-bridge/ProtocolTests.cpp"
        "${wxl_ext_dir}/Json.cpp"
        "${wxl_ext_dir}/Protocol.cpp")
    target_include_directories(wcs-bridge-tests PRIVATE "${wxl_ext_dir}")
    target_compile_definitions(wcs-bridge-tests PRIVATE ${WXL_DEFS})
    add_test(NAME wcs-bridge-protocol COMMAND wcs-bridge-tests)

    add_executable(wcs-bridge-websocket-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wcs-bridge/WebSocketTests.cpp"
        "${wxl_ext_dir}/Json.cpp"
        "${wxl_ext_dir}/Protocol.cpp"
        "${wxl_ext_dir}/Pairing.cpp"
        "${wxl_ext_dir}/WebSocketServer.cpp")
    target_include_directories(wcs-bridge-websocket-tests PRIVATE "${wxl_ext_dir}")
    target_compile_definitions(wcs-bridge-websocket-tests PRIVATE ${WXL_DEFS})
    target_link_libraries(wcs-bridge-websocket-tests PRIVATE ws2_32 bcrypt crypt32)
    add_test(NAME wcs-bridge-websocket COMMAND wcs-bridge-websocket-tests)
    set_tests_properties(wcs-bridge-websocket PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
