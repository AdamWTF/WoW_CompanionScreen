# WarcraftXL. Two artifact families share this tree and cannot share one toolchain platform, so the file
# is platform-aware: configure -A Win32 for the injected DLL + patcher + proxy (32-bit), and -A x64 with
# -DWXL_BUILD_HOST=ON for the asset host (64-bit). build.ps1 drives both.
# Copyright (C) 2026 WarcraftXL. GPLv3 (see source headers).

cmake_minimum_required(VERSION 3.20)
# C is enabled for the vendored C libraries pulled in via add_subdirectory (MinHook; StormLib in the host).
project(WarcraftXL LANGUAGES CXX C)

# Static CRT everywhere: the DLLs load inside the player's Wow.exe and the host runs standalone on
# machines that have no reason to carry the toolset's brand-new VC++ redistributable. A dynamic CRT
# turns a missing/stale redist into a loader failure at process start (0xc0000142) with no log at all.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# The 64-bit asset host (WarcraftXLHost.exe) is opt-in: OFF by default so a contributor who only wants the
# DLL needs no x64 toolchain and pays no build/deploy cost. build.ps1 -Host flips it on in an x64 configure.
option(WXL_BUILD_HOST "Build WarcraftXLHost.exe (64-bit asset host)" OFF)

# The modern-assets pipelines (ex wxl-modern-assets, now src/engine/assets/) are always compiled; their
# per-format enablement is a compile switch in src/config.hpp (wxl::features::modern{M2,WMO,ADT}Support),
# honored by each module's registrar via `if constexpr`. No CMake exclusion — see config.hpp.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # feeds the IDE the real C++20 flags

set(WXL_DEFS WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)

# Client directory to deploy the built artifacts into. Passed by build.ps1 (-DCLIENT_PATH=...) and cached.
set(CLIENT_PATH "" CACHE PATH "Client folder the built artifacts are copied into after each build")

# --- SDK boundary --------------------------------------------------------------------------------
option(WXL_STRICT_SDK_BOUNDARY "Fail the configure when a file outside src/game includes offsets/" OFF)

file(GLOB_RECURSE WXL_BOUNDARY_SCAN CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
list(FILTER WXL_BOUNDARY_SCAN EXCLUDE REGEX "/src/(game|offsets)/")

set(WXL_BOUNDARY_VIOLATIONS "")
foreach(wxl_file IN LISTS WXL_BOUNDARY_SCAN)
    file(STRINGS "${wxl_file}" wxl_hit REGEX "^[ \t]*#include[ \t]+\"offsets/")
    if(wxl_hit)
        file(RELATIVE_PATH wxl_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${wxl_file}")
        list(APPEND WXL_BOUNDARY_VIOLATIONS "${wxl_rel}")
    endif()
endforeach()

list(LENGTH WXL_BOUNDARY_VIOLATIONS WXL_BOUNDARY_COUNT)
if(WXL_BOUNDARY_COUNT GREATER 0)
    string(REPLACE ";" "\n  " WXL_BOUNDARY_PRETTY "${WXL_BOUNDARY_VIOLATIONS}")
    if(WXL_STRICT_SDK_BOUNDARY)
        message(FATAL_ERROR
            "SDK boundary: ${WXL_BOUNDARY_COUNT} file(s) outside src/game/ include offsets/:\n  ${WXL_BOUNDARY_PRETTY}")
    endif()
    message(STATUS "SDK boundary: ${WXL_BOUNDARY_COUNT} file(s) outside src/game/ still include offsets/")
endif()

# =============================================================================
# 32-bit family (injected into the client): WarcraftXL.dll + wxl-patcher.exe + d3d9.dll
# =============================================================================
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    # --- Automatic, recursive source discovery (no hand-written paths) -----------------------------
    # CONFIGURE_DEPENDS re-globs at build time, so dropping a file (or a whole script) needs no edit here.
    # The DLL excludes the patcher/host trees and the proxy (engine/gpu builds its own target below).
    file(GLOB_RECURSE WXL_CORE_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
    list(FILTER WXL_CORE_SRC EXCLUDE REGEX "/src/(patcher|host)/|/src/engine/gpu/")

    # =========================================================================
    # Target 1: WarcraftXL.dll  (32-bit, injected) = engine + features + assets pipelines
    #   The modern-assets DLL + cross-target-shared halves now live in src/engine/assets/ and are caught
    #   by the src/*.cpp glob above (the shared half also compiles into the 64-bit host, added there).
    # =========================================================================
    # FlatBuffers runtime (flexbuffers string utils) for the host IPC client (engine/storage).
    set(WXL_FLATBUFFERS "${CMAKE_CURRENT_SOURCE_DIR}/deps/flatbuffers/src/util.cpp")

    # Dear ImGui: the in-game tuning panels (engine/ui). Listed explicitly rather than globbed -- only
    # the core plus the DX9 and Win32 backends are vendored, and the demo window is deliberately not.
    set(WXL_IMGUI
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends/imgui_impl_dx9.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends/imgui_impl_win32.cpp")

    add_library(WarcraftXL SHARED ${WXL_CORE_SRC} ${WXL_FLATBUFFERS} ${WXL_IMGUI})
    set_target_properties(WarcraftXL PROPERTIES OUTPUT_NAME "WarcraftXL" PREFIX "")
    target_include_directories(WarcraftXL PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"   # the extension ABI, shared verbatim with extensions
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/minhook/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/flatbuffers/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends")

    add_subdirectory(deps/minhook)
    target_link_libraries(WarcraftXL PRIVATE minhook d3d9 shell32)
    target_compile_definitions(WarcraftXL PRIVATE ${WXL_DEFS})

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${WXL_CORE_SRC})

    # Deploy: copy the freshly linked DLL into the client folder.
    if(CLIENT_PATH)
        add_custom_command(TARGET WarcraftXL POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:WarcraftXL>" "${CLIENT_PATH}/WarcraftXL.dll"
            COMMENT "Deploy WarcraftXL.dll -> ${CLIENT_PATH}")
    endif()

    # =========================================================================
    # Target 2: wxl-patcher.exe  = the offline PE patcher (incl. the ex-wxl-no-gluexml example under
    #   src/patcher/, now caught by the src/patcher glob below).
    # =========================================================================
    file(GLOB_RECURSE WXL_PATCHER_CORE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/patcher/*.cpp")

    # src/common/ compiles into every binary (leveled logger, config, shared helpers): the patcher and
    # proxy do not link the engine, and the host is another architecture, so the shared infrastructure
    # is object-level, not a link dependency.
    file(GLOB WXL_COMMON_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp")

    add_executable(wxl-patcher ${WXL_PATCHER_CORE} ${WXL_COMMON_SRC})
    set_target_properties(wxl-patcher PROPERTIES OUTPUT_NAME "wxl-patcher")
    target_include_directories(wxl-patcher PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(wxl-patcher PRIVATE ${WXL_DEFS})

    # =========================================================================
    # Target 3: d3d9.dll  = the native proxy (system-d3d9 passthrough + runtime loader)
    # =========================================================================
    file(GLOB WXL_GPU_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/engine/gpu/*.cpp")

    add_library(d3d9 SHARED ${WXL_GPU_SRC} ${WXL_COMMON_SRC})
    set_target_properties(d3d9 PROPERTIES OUTPUT_NAME "d3d9" PREFIX "")
    target_include_directories(d3d9 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    # The proxy shadows a static import of Wow.exe, so a hard dependency of the proxy is a hard
    # dependency of the game. It resolves the system d3d9 at runtime and links nothing beyond the CRT.
    target_link_options(d3d9 PRIVATE "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/engine/gpu/Proxy.def")
    target_compile_definitions(d3d9 PRIVATE ${WXL_DEFS})

    # Deploy: copy the freshly linked proxy into the client folder after every build.
    if(CLIENT_PATH)
        add_custom_command(TARGET d3d9 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:d3d9>" "${CLIENT_PATH}/d3d9.dll"
            COMMENT "Deploy d3d9.dll -> ${CLIENT_PATH}")
    endif()

    # =========================================================================
    # Target 4+: one DLL per folder under extensions/, deployed to Extensions/<Name>/<Name>.dll and
    #   discovered by the core at runtime. They link nothing of the core -- the SDK is header-only and
    #   compiles in, and everything needing arbitration arrives through the API table -- so an
    #   extension built here has exactly the same relationship to the core as a third party's.
    # =========================================================================
    file(GLOB WXL_EXTENSION_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/extensions/*")
    foreach(wxl_ext_dir IN LISTS WXL_EXTENSION_DIRS)
        if(NOT IS_DIRECTORY "${wxl_ext_dir}")
            continue()
        endif()
        get_filename_component(wxl_ext_name "${wxl_ext_dir}" NAME)
        file(GLOB_RECURSE WXL_EXT_SRC CONFIGURE_DEPENDS "${wxl_ext_dir}/*.cpp")
        if(NOT WXL_EXT_SRC)
            continue()
        endif()

        add_library(${wxl_ext_name} SHARED ${WXL_EXT_SRC})
        set_target_properties(${wxl_ext_name} PROPERTIES OUTPUT_NAME "${wxl_ext_name}" PREFIX "")
        target_include_directories(${wxl_ext_name} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/src")
        target_compile_definitions(${wxl_ext_name} PRIVATE ${WXL_DEFS})

        if(CLIENT_PATH)
            add_custom_command(TARGET ${wxl_ext_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CLIENT_PATH}/Extensions/${wxl_ext_name}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${wxl_ext_name}>"
                        "${CLIENT_PATH}/Extensions/${wxl_ext_name}/${wxl_ext_name}.dll"
                COMMENT "Deploy ${wxl_ext_name}.dll -> ${CLIENT_PATH}/Extensions/${wxl_ext_name}")
        endif()
    endforeach()
endif()

# =============================================================================
# 64-bit family (opt-in): WarcraftXLHost.exe = archive owner + IPC transport.
#   Format-blind: src/host/ (incl. assets/ host handlers + resolver/) + the cross-target byte transforms
#   shared with the DLL in src/engine/assets/shared/. StormLib + FlatBuffers (deps/). Requires x64.
# =============================================================================
if(WXL_BUILD_HOST AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    # StormLib (vendored) supplies MPQ reading. Static, read-only use; no install/tests.
    set(STORM_SKIP_INSTALL ON CACHE BOOL "" FORCE)
    set(STORM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/stormlib" "${CMAKE_BINARY_DIR}/stormlib")

    # src/host/ = host core + the ex-module host handlers (assets/, resolver/), all self-registering.
    file(GLOB_RECURSE WXL_HOST_CORE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/host/*.cpp")
    # The cross-target byte transforms shared with the DLL (ex scripts/*/shared) now live here.
    file(GLOB_RECURSE WXL_HOST_SHARED CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/engine/assets/shared/*.cpp")

    # The shared infrastructure (leveled logger, config) compiles straight into the host too.
    file(GLOB WXL_COMMON_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp")

    add_executable(WarcraftXLHost
        ${WXL_HOST_CORE}
        ${WXL_HOST_SHARED}
        ${WXL_COMMON_SRC}
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/flatbuffers/src/util.cpp")
    set_target_properties(WarcraftXLHost PROPERTIES OUTPUT_NAME "WarcraftXLHost")
    target_include_directories(WarcraftXLHost PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/host"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/flatbuffers/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/deps/stormlib/src")
    target_link_libraries(WarcraftXLHost PRIVATE storm)
    target_compile_definitions(WarcraftXLHost PRIVATE ${WXL_DEFS} WXL_HOST=1)

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${WXL_HOST_CORE} ${WXL_HOST_SHARED})

    # Deploy the host into the client's Utils folder after build.
    if(CLIENT_PATH)
        add_custom_command(TARGET WarcraftXLHost POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CLIENT_PATH}/Utils"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:WarcraftXLHost>" "${CLIENT_PATH}/Utils/WarcraftXLHost.exe"
            COMMENT "Deploy WarcraftXLHost.exe -> ${CLIENT_PATH}/Utils")
    endif()
endif()

# A 64-bit configure that did not ask for the host builds nothing; nudge toward the right flags.
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WXL_BUILD_HOST)
    message(WARNING "x64 configure without -DWXL_BUILD_HOST=ON builds nothing; the 32-bit family needs -A Win32.")
endif()
