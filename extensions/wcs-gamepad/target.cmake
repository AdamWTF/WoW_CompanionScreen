if(BUILD_TESTING)
    add_executable(wcs-gamepad-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wcs-gamepad/ControllerTests.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad/ControllerConfig.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad/ControllerGameplay.cpp")
    target_include_directories(wcs-gamepad-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(wcs-gamepad-tests PRIVATE ${WXL_DEFS})
    target_link_libraries(wcs-gamepad-tests PRIVATE user32)
    add_test(NAME wcs-gamepad-processing COMMAND wcs-gamepad-tests)

    add_test(NAME wcs-gamepad-input-commands
        COMMAND "${CMAKE_COMMAND}"
            "-DGAME_INPUT_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad/GameInput.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/wcs-gamepad/GameInputSourceTests.cmake")

    add_executable(wcs-gamepad-camera-view-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wcs-gamepad/CameraViewTests.cpp")
    target_include_directories(wcs-gamepad-camera-view-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(wcs-gamepad-camera-view-tests PRIVATE ${WXL_DEFS})
    add_test(NAME wcs-gamepad-camera-view COMMAND wcs-gamepad-camera-view-tests)

    find_program(WXL_LUA_EXECUTABLE NAMES lua5.1 lua luajit)
    if(WXL_LUA_EXECUTABLE)
        add_test(NAME wcs-ui-navigation
            COMMAND "${WXL_LUA_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/wcs-gamepad/UINavigationTests.lua"
                "${CMAKE_CURRENT_SOURCE_DIR}/addon/WoWCompanionScreen/UI/UINavigation.lua")
    endif()

    add_executable(wcs-gamepad-smart-interact-tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wcs-gamepad/SmartInteractTests.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad/SmartInteract.cpp")
    target_include_directories(wcs-gamepad-smart-interact-tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad")
    target_compile_definitions(wcs-gamepad-smart-interact-tests PRIVATE ${WXL_DEFS})
    add_test(NAME wcs-gamepad-smart-interact COMMAND wcs-gamepad-smart-interact-tests)
endif()

if(CLIENT_PATH)
    add_custom_command(TARGET wcs-gamepad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad/gamecontrollerdb.txt"
            "${CLIENT_PATH}/Extensions/wcs-gamepad/gamecontrollerdb.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad/wcs-gamepad.cfg.example"
            "${CLIENT_PATH}/Extensions/wcs-gamepad/wcs-gamepad.cfg.example"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/extensions/wcs-gamepad/gamecontrollerdb.LICENSE.txt"
            "${CLIENT_PATH}/Extensions/wcs-gamepad/gamecontrollerdb.LICENSE.txt")
endif()
