if(BUILD_TESTING)
    add_executable(wxl-gamepad-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wxl-gamepad/ControllerTests.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wxl-gamepad/ControllerConfig.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wxl-gamepad/ControllerGameplay.cpp")
    target_include_directories(wxl-gamepad-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wxl-gamepad"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(wxl-gamepad-tests PRIVATE ${WXL_DEFS})
    target_link_libraries(wxl-gamepad-tests PRIVATE user32)
    add_test(NAME wxl-gamepad-processing COMMAND wxl-gamepad-tests)

    add_executable(wxl-gamepad-smart-interact-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wxl-gamepad/SmartInteractTests.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wxl-gamepad/SmartInteract.cpp")
    target_include_directories(wxl-gamepad-smart-interact-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wxl-gamepad")
    target_compile_definitions(wxl-gamepad-smart-interact-tests PRIVATE ${WXL_DEFS})
    add_test(NAME wxl-gamepad-smart-interact COMMAND wxl-gamepad-smart-interact-tests)
endif()

if(CLIENT_PATH)
    add_custom_command(TARGET wxl-gamepad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wxl-gamepad/gamecontrollerdb.txt"
            "${CLIENT_PATH}/Extensions/wxl-gamepad/gamecontrollerdb.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wxl-gamepad/wxl-gamepad.cfg.example"
            "${CLIENT_PATH}/Extensions/wxl-gamepad/wxl-gamepad.cfg.example"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wxl-gamepad/gamecontrollerdb.LICENSE.txt"
            "${CLIENT_PATH}/Extensions/wxl-gamepad/gamecontrollerdb.LICENSE.txt")
endif()
