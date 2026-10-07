# boom_boom_rocket - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(boom_boom_rocket LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BBR_GAME_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/private/input" CACHE PATH "Private extracted original game")
set(BBR_SDK_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/out/sdk-install" CACHE PATH "Pinned patched SDK install prefix")
list(PREPEND CMAKE_PREFIX_PATH "${BBR_SDK_PREFIX}")
# Prevent assert/__FILE__ strings from disclosing the developer's checkout.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=project/")
endif()

include(generated/rexglue.cmake)

# Sources
set(BOOMBOOMROCKET_SOURCES
    src/main.cpp
    src/local_multiplayer.cpp
)

if(WIN32)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/game.ico"
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/New-FireworkIcon.ps1"
            -OutputPath "${CMAKE_CURRENT_BINARY_DIR}/game.ico" -Design Game
        DEPENDS tools/New-FireworkIcon.ps1 VERBATIM)
    find_program(BBR_RC rc)
    if(NOT BBR_RC)
        file(GLOB BBR_KIT_RCS "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/*/x64/rc.exe")
        list(SORT BBR_KIT_RCS COMPARE NATURAL ORDER DESCENDING)
        list(LENGTH BBR_KIT_RCS BBR_KIT_COUNT)
        if(BBR_KIT_COUNT EQUAL 0)
            message(FATAL_ERROR "Windows SDK resource compiler not found; install the Windows SDK or set BBR_RC.")
        endif()
        list(GET BBR_KIT_RCS 0 BBR_RC)
    endif()
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/game.res"
        COMMAND "${BBR_RC}" /nologo /i "${CMAKE_CURRENT_BINARY_DIR}"
            /fo "${CMAKE_CURRENT_BINARY_DIR}/game.res" "${CMAKE_CURRENT_SOURCE_DIR}/src/game.rc"
        DEPENDS src/game.rc "${CMAKE_CURRENT_BINARY_DIR}/game.ico" VERBATIM)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/game.res" PROPERTIES EXTERNAL_OBJECT TRUE)
    list(APPEND BOOMBOOMROCKET_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/game.res")
    add_executable(boom_boom_rocket WIN32 ${BOOMBOOMROCKET_SOURCES})
    target_include_directories(boom_boom_rocket PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
else()
    add_executable(boom_boom_rocket ${BOOMBOOMROCKET_SOURCES})
endif()

rexglue_setup_target(boom_boom_rocket GPU_PLUGINS xenos)

# Explicit diagnostic target; never included in a portable payload.
add_executable(bbr_input_tests EXCLUDE_FROM_ALL tests/keyboard_driver_test.cpp)
target_include_directories(bbr_input_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(bbr_input_tests PRIVATE rex::runtime)
add_executable(bbr_profile_tests EXCLUDE_FROM_ALL tests/profile_welcome_test.cpp)
target_include_directories(bbr_profile_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_executable(bbr_timing_tests EXCLUDE_FROM_ALL tests/timing_calibration_test.cpp)
target_include_directories(bbr_timing_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# Isolated UI probe supplies a second synthetic pad using IJKL/U/O/N/M/F6.
# Production never includes this driver and requires two physical controllers.
add_executable(boom_boom_rocket_multiplayer_probe EXCLUDE_FROM_ALL WIN32 ${BOOMBOOMROCKET_SOURCES})
target_include_directories(boom_boom_rocket_multiplayer_probe PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(boom_boom_rocket_multiplayer_probe PRIVATE BBR_MULTIPLAYER_TEST=1)
rexglue_setup_target(boom_boom_rocket_multiplayer_probe GPU_PLUGINS xenos)

add_executable(bbr_multiplayer_tests EXCLUDE_FROM_ALL tests/local_multiplayer_test.cpp src/local_multiplayer.cpp)
target_include_directories(bbr_multiplayer_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(bbr_multiplayer_tests PRIVATE rex::runtime)
