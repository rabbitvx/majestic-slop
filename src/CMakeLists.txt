cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(hello LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(HELLO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(HELLO_THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

option(HELLO_ENABLE_RELEASE_LOGS "Enable runtime logs in release builds" OFF)
option(HELLO_ENABLE_CRASH_LOGS "Enable runtime crash log files in release builds" OFF)

file(GLOB_RECURSE HELLO_SOURCES CONFIGURE_DEPENDS
    "${HELLO_SRC_DIR}/*.cpp"
    "${HELLO_THIRDPARTY_DIR}/dependencies/minhook/src/*.c"
    "${HELLO_THIRDPARTY_DIR}/kiero/*.cpp"
    "${HELLO_THIRDPARTY_DIR}/imgui/*.cpp"
)

list(FILTER HELLO_SOURCES EXCLUDE REGEX ".*imgui_demo\\.cpp$")
list(FILTER HELLO_SOURCES EXCLUDE REGEX ".*[/\\\\]imgui_freetype\\.cpp$")
list(FILTER HELLO_SOURCES EXCLUDE REGEX ".*[/\\\\]runtime[/\\\\](init|tasks)\\.cpp$")
list(FILTER HELLO_SOURCES EXCLUDE REGEX ".*[/\\\\]ui[/\\\\]preview[/\\\\]model_preview\\.cpp$")

add_library(hello SHARED ${HELLO_SOURCES})

target_sources(hello PRIVATE
    "${HELLO_SRC_DIR}/ui/preview/model_preview.cpp"
)

set_target_properties(hello PROPERTIES
    OUTPUT_NAME "dxilconv"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
)

target_include_directories(hello PRIVATE
    "${HELLO_SRC_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${HELLO_SRC_DIR}/core"
    "${HELLO_SRC_DIR}/render"
    "${HELLO_SRC_DIR}/ui/menu"
    "${HELLO_THIRDPARTY_DIR}"
    "${HELLO_THIRDPARTY_DIR}/imgui"
    "${HELLO_THIRDPARTY_DIR}/simplemath"
    "${HELLO_THIRDPARTY_DIR}/dependencies"
    "${HELLO_THIRDPARTY_DIR}/dependencies/minhook/include"
    "${HELLO_THIRDPARTY_DIR}/kiero"
    "${HELLO_THIRDPARTY_DIR}/nlohmann_json/build/native/include"
)

target_compile_definitions(hello PRIVATE
    HELLO_EXPORTS
    _WINDOWS
    _USRDLL
    IMGUI_DEFINE_MATH_OPERATORS
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
    _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    _ENABLE_EXTENDED_ALIGNED_STORAGE
)

# MSVC flags — maximum obfuscation without OLLVM
foreach(flag_var CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG)
    string(REPLACE "/RTC1" "" ${flag_var} "${${flag_var}}")
    string(REPLACE "/RTCs" "" ${flag_var} "${${flag_var}}")
    string(REPLACE "/RTCu" "" ${flag_var} "${${flag_var}}")
    string(REPLACE "/MDd" "/MT" ${flag_var} "${${flag_var}}")
    string(REPLACE "/MTd" "/MT" ${flag_var} "${${flag_var}}")
endforeach()

target_compile_options(hello PRIVATE
    /EHsc /FS /Zc:__cplusplus /bigobj /utf-8
    # Size optimization — changes control flow, shrinks code
    $<$<CONFIG:Release>:/O1>
    $<$<CONFIG:Release>:/Ob2>
    $<$<CONFIG:Debug>:/Od>
    $<$<CONFIG:Debug>:/Ob0>
)

target_compile_definitions(hello PRIVATE
    $<$<CONFIG:Debug>:HELLO_DEBUG_BUILD>
    $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    $<$<CONFIG:Debug>:_HAS_ITERATOR_DEBUGGING=0>
    $<$<CONFIG:Release>:HELLO_PRODUCTION_BUILD>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${HELLO_ENABLE_RELEASE_LOGS}>>:HELLO_ENABLE_RUNTIME_LOGS>
    $<$<AND:$<CONFIG:Release>,$<BOOL:${HELLO_ENABLE_CRASH_LOGS}>>:HELLO_ENABLE_CRASH_LOGS>
)

target_link_options(hello PRIVATE
    # Whole program optimization — cross-module inlining
    /LTCG
    # Remove unreferenced code + fold identical functions
    /OPT:REF
    /OPT:ICF
    # No incremental link in release
    $<$<CONFIG:Release>:/INCREMENTAL:NO>
    $<$<CONFIG:Debug>:/DEBUG>
    $<$<CONFIG:Debug>:/PDBALTPATH:debug.pdb>
)

target_link_libraries(hello PRIVATE
    d3d11 Dwmapi d3dcompiler ws2_32 winmm bcrypt advapi32 crypt32
)
