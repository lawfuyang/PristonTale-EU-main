cmake_minimum_required(VERSION 3.20)

# Enforce 32-bit (Win32) — required by this project
set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "Target platform" FORCE)

project(PristonTale VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_compile_options(/wd4995 /wd4290 /wd4091 /wd4005 /wd4065 /wd4409 /wd4996 /MP)
add_link_options(/MACHINE:X86)

set(SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shared")
set(GAME_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/game/game")
set(DXSDK_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/deps/dks")

# Shared sources compiled in the server (per server.vcxproj)
set(SHARED_SERVER
    ${SHARED_DIR}/CApplication.cpp
    ${SHARED_DIR}/CConfig.cpp
    ${SHARED_DIR}/CConfigFile.cpp
    ${SHARED_DIR}/CConfigFileReader.cpp
    ${SHARED_DIR}/CConfigFileWriter.cpp
    ${SHARED_DIR}/CCore.cpp
    ${SHARED_DIR}/character.cpp
    ${SHARED_DIR}/chat.cpp
    ${SHARED_DIR}/CMutex.cpp
    ${SHARED_DIR}/Coin.cpp
    ${SHARED_DIR}/CoinShop.cpp
    ${SHARED_DIR}/CoinShopItem.cpp
    ${SHARED_DIR}/CoinShopTab.cpp
    ${SHARED_DIR}/common.cpp
    ${SHARED_DIR}/CProcess.cpp
    ${SHARED_DIR}/CReader.cpp
    ${SHARED_DIR}/CReaderFile.cpp
    ${SHARED_DIR}/CSystem.cpp
    ${SHARED_DIR}/CWindow.cpp
    ${SHARED_DIR}/CWorld.cpp
    ${SHARED_DIR}/debug.cpp
    ${SHARED_DIR}/def.cpp
    ${SHARED_DIR}/dice.cpp
    ${SHARED_DIR}/item.cpp
    ${SHARED_DIR}/map.cpp
    ${SHARED_DIR}/party.cpp
    ${SHARED_DIR}/point.cpp
    ${SHARED_DIR}/quest.cpp
    ${SHARED_DIR}/socket.cpp
    ${SHARED_DIR}/sound.cpp
    ${SHARED_DIR}/strings.cpp
    ${SHARED_DIR}/unit.cpp
    ${SHARED_DIR}/user.cpp
)

# Shared sources compiled in the client (per game.vcxproj).
# NOTE: intentionally differs from SHARED_SERVER — the client additionally
# compiles CLua/CTimer/Event/EventHandler/EXEMatrix/Node/skill/sky/Timer/
# X3DEasings, and does NOT compile user.cpp.
set(SHARED_GAME
    ${SHARED_DIR}/CApplication.cpp
    ${SHARED_DIR}/character.cpp
    ${SHARED_DIR}/chat.cpp
    ${SHARED_DIR}/CLua.cpp
    ${SHARED_DIR}/Coin.cpp
    ${SHARED_DIR}/CoinShop.cpp
    ${SHARED_DIR}/CoinShopItem.cpp
    ${SHARED_DIR}/CoinShopTab.cpp
    ${SHARED_DIR}/common.cpp
    ${SHARED_DIR}/CProcess.cpp
    ${SHARED_DIR}/CReader.cpp
    ${SHARED_DIR}/CReaderFile.cpp
    ${SHARED_DIR}/CSystem.cpp
    ${SHARED_DIR}/CTimer.cpp
    ${SHARED_DIR}/CWindow.cpp
    ${SHARED_DIR}/CWorld.cpp
    ${SHARED_DIR}/debug.cpp
    ${SHARED_DIR}/def.cpp
    ${SHARED_DIR}/dice.cpp
    ${SHARED_DIR}/Event.cpp
    ${SHARED_DIR}/EventHandler.cpp
    ${SHARED_DIR}/EXEMatrix.cpp
    ${SHARED_DIR}/item.cpp
    ${SHARED_DIR}/CMutex.cpp
    ${SHARED_DIR}/map.cpp
    ${SHARED_DIR}/Node.cpp
    ${SHARED_DIR}/party.cpp
    ${SHARED_DIR}/point.cpp
    ${SHARED_DIR}/quest.cpp
    ${SHARED_DIR}/skill.cpp
    ${SHARED_DIR}/sky.cpp
    ${SHARED_DIR}/socket.cpp
    ${SHARED_DIR}/sound.cpp
    ${SHARED_DIR}/strings.cpp
    ${SHARED_DIR}/Timer.cpp
    ${SHARED_DIR}/unit.cpp
    ${SHARED_DIR}/X3DEasings.cpp
)

# ============================================================================
# Server DLL
# ============================================================================
file(GLOB SERVER_CPP "${CMAKE_CURRENT_SOURCE_DIR}/Server/server/*.cpp")
list(FILTER SERVER_CPP EXCLUDE REGEX "/SQLAPI\\.cpp$")
list(FILTER SERVER_CPP EXCLUDE REGEX "/writeServerField\\.cpp$")
list(FILTER SERVER_CPP EXCLUDE REGEX "/writeClientField\\.cpp$")

add_library(server SHARED ${SERVER_CPP} ${SHARED_SERVER})

target_include_directories(server PRIVATE
    ${SHARED_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/Server/server"
)
target_compile_definitions(server PRIVATE
    _SERVER;SERVER_EXPORTS;WIN32;_WINDOWS;_USRDLL
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)
target_precompile_headers(server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Server/server/stdafx.h")
target_link_libraries(server PRIVATE
    odbc32 odbccp32
    ws2_32 winmm wininet imagehlp imm32
)
target_link_options(server PRIVATE /DEF:"${CMAKE_CURRENT_SOURCE_DIR}/Server/server/export.def")
set_target_properties(server PROPERTIES
    PREFIX "" SUFFIX ".dll" OUTPUT_NAME "server"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Server"
    WIN32_EXECUTABLE TRUE
)

# Post-build: auto-deploy to both server directories
add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:server>"
        "${CMAKE_CURRENT_SOURCE_DIR}/Files/Server/login-server/server.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:server>"
        "${CMAKE_CURRENT_SOURCE_DIR}/Files/Server/game-server/server.dll"
    COMMENT "Deploying server.dll to login-server and game-server..."
)

# ============================================================================
# Game (client) DLL
#
# IMPORTANT: this builds game.dll, NOT Game.exe. There is no source for
# Game.exe in this repository — it is a prebuilt, closed-source binary that
# statically imports WinMain from game.dll and calls it as its entry point.
# game.dll is therefore a hook/mod layer that patches Game.exe in memory at
# ~4000 hardcoded absolute addresses, so a build is only valid against the
# exact Game.exe it was written for. Back up the runtime DLL before replacing.
# ============================================================================
file(GLOB GAME_CPP "${GAME_DIR}/*.cpp")

# Orphaned sources: present on disk but deliberately NOT in game.vcxproj.
# These are dead/experimental or alternative implementations; compiling them
# causes duplicate-symbol and unresolved-external errors. Keep this list in
# sync with game.vcxproj.
set(GAME_EXCLUDE
    CPremiumHandler DX DXAudioEngine DXColor DXFontFactory DXFunctions
    DXLens DXMaterial DXMesh DXMeshPart DXModel DXModelFactory DXMusic
    DXSound DXSoundCaptureDevice DXSoundDevice DXSoundFactory DXSoundManager
    DXSoundStream DXTerrain DXTerrainFactory FullZoomMap PremiumModel
    PremiumView TestUI UIController UIFont UILine UIMessageBox UIScroller
    writeClientField
)
foreach(name ${GAME_EXCLUDE})
    list(FILTER GAME_CPP EXCLUDE REGEX "/${name}\\.cpp$")
endforeach()

add_library(game SHARED ${GAME_CPP} ${SHARED_GAME} "${GAME_DIR}/resource.rc")

target_include_directories(game PRIVATE
    ${SHARED_DIR}
    ${GAME_DIR}
    "${DXSDK_DIR}/Include"
)
target_compile_definitions(game PRIVATE
    _GAME;WIN32;_WINDOWS;_USRDLL;GAME_EXPORTS
    $<$<CONFIG:Debug>:_DEBUG;_ANIM>
    $<$<CONFIG:Release>:NDEBUG>
)
target_precompile_headers(game PRIVATE "${GAME_DIR}/stdafx.h")

# DLL.cpp and stdafx.cpp opt out of the PCH in game.vcxproj; stdafx.cpp also
# overrides global operator new/delete, which must not be pulled into the PCH.
set_source_files_properties(
    "${GAME_DIR}/DLL.cpp"
    "${GAME_DIR}/stdafx.cpp"
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

# lua.lib and discord-rpc.lib are vendored next to the client sources; the
# DirectX libs come from deps/dks. Most other libs are pulled in via
# #pragma comment(lib, ...) in stdafx.h.
target_link_directories(game PRIVATE
    "${DXSDK_DIR}/Lib/x86"
    ${GAME_DIR}
)
target_link_libraries(game PRIVATE
    discord-rpc
    ws2_32 winmm wininet imagehlp imm32
    shell32 rpcrt4 shlwapi vfw32 wbemuuid strmiids
)
target_link_options(game PRIVATE
    /DEF:"${GAME_DIR}/export.def"
    /SAFESEH:NO
    /OPT:NOLBR
)
set_target_properties(game PROPERTIES
    PREFIX "" SUFFIX ".dll" OUTPUT_NAME "game"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Game"
    WIN32_EXECUTABLE TRUE
)

# Post-build: auto-deploy to the client runtime directory.
#
# Files/Game/game.dll is a live runtime artifact that is NOT tracked by Git
# (see .gitignore), so an overwrite is unrecoverable. A one-time .orig backup
# is therefore taken before the first deploy. Deployment is opt-in for the
# same reason: a game.dll built against a different Game.exe will crash.
option(DEPLOY_GAME_DLL "Copy the built game.dll into Files/Game after build" OFF)

if(DEPLOY_GAME_DLL)
    set(GAME_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Files/Game")

    # Preserve the original shipped DLL exactly once, before it is ever touched.
    if(EXISTS "${GAME_RUNTIME_DIR}/game.dll"
       AND NOT EXISTS "${GAME_RUNTIME_DIR}/game.dll.orig")
        configure_file(
            "${GAME_RUNTIME_DIR}/game.dll"
            "${GAME_RUNTIME_DIR}/game.dll.orig"
            COPYONLY
        )
        message(STATUS "Backed up original runtime DLL to Files/Game/game.dll.orig")
    endif()

    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:game>"
            "${GAME_RUNTIME_DIR}/game.dll"
        COMMENT "Deploying game.dll to Files/Game..."
    )
else()
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
            "game.dll built. Re-run cmake with -DDEPLOY_GAME_DLL=ON to deploy to Files/Game."
    )
endif()
