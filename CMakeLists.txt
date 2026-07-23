cmake_minimum_required(VERSION 3.20)
project(PristonTale VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "32-bit required. Use: cmake -B build -A Win32")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_compile_options(/wd4995 /wd4290 /wd4091 /wd4005 /wd4065 /wd4409 /wd4996 /MP)

set(SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shared")

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
