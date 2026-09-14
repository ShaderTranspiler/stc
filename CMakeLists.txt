# TODO: set up CMakePresets instead of CMakeSettings

cmake_minimum_required(VERSION 3.16)

include(FetchContent)

set(STC_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(STC_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(STC_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(STC_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(STC_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test)

# Version info

# set current project version in these two lines
# cascades through the entire codebase, treated as a single source of truth
project(ShaderTranspilerCore VERSION 0.8.5)
set(STC_VERSION_SUFFIX "")

if(NOT STC_VERSION_SUFFIX STREQUAL "")
    if (NOT STC_VERSION_SUFFIX MATCHES "^rc\\.[0-9]+$")
        message(FATAL_ERROR "Invalid STC_VERSION_SUFFIX: '${STC_VERSION_SUFFIX}'.\n"
        "Must be empty for releases, or have the form rc.N for release candidates (e.g. rc.1).")
    endif()

    set(STC_VERSION "${PROJECT_VERSION}-${STC_VERSION_SUFFIX}")
else()
    set(STC_VERSION "${PROJECT_VERSION}")
endif()

set(STC_SOVERSION "${PROJECT_VERSION_MAJOR}") # ! ABI version is linked to semver's major version

if (DEFINED STC_COMMIT_HASH AND NOT STC_COMMIT_HASH STREQUAL "")
    set(STC_GIT_EXECUTABLE "")
    message(STATUS "Using explicitly provided commit hash: ${STC_COMMIT_HASH}")
else()
    set(STC_COMMIT_HASH "")
    
    find_package(Git QUIET)
    if (Git_FOUND)
        set(STC_GIT_EXECUTABLE "${GIT_EXECUTABLE}")
        message(STATUS "Commit hash not specified, but Git is available on the system: the latest commit hash will be acquired automatically before every build")
    else()
        set(STC_GIT_EXECUTABLE "")
        message(WARNING "Commit hash not specified and Git is not available on the system: commit hash metadata won't be available")
    endif()
endif()

if (DEFINED STC_BUILD_TIMESTAMP AND NOT STC_BUILD_TIMESTAMP STREQUAL "")
    message(STATUS "Using explicitly provided build timestamp: ${STC_BUILD_TIMESTAMP}")
else()
    set(STC_BUILD_TIMESTAMP "")
    message(STATUS "No explicit build timestamp provided, system time will be queried automatically before every build")
endif()

message(STATUS "Build version is ${STC_VERSION}")

# Config

# fallback for users and build pipelines that prefer BUILD_TESTING over a prefixed switch 
# an explicit -DSTC_BUILD_TESTS=<value> parameter still overwrites BUILD_TESTING
set(STC_BUILD_TESTS_DEFAULT OFF)
if (DEFINED BUILD_TESTING)
    set(STC_BUILD_TESTS_DEFAULT ${BUILD_TESTING})
endif()

option(STC_BUILD_CLI "Enable CLI target (disabling can save a bit of time in some cases, where it's enough to build the lib)" ON)
option(STC_BUILD_DOCS "Enable docs-related targets." OFF)
option(STC_BUILD_TESTS "Enable targets for running unit and other tests suites (fetches Catch2 if not available on host)." ${STC_BUILD_TESTS_DEFAULT})
option(STC_BUILD_SANDBOX "Enable sandbox targets" OFF)
option(STC_ENABLE_PROFILING "Enables downloading and linking Tracy through FetchContent, and defines the STC_PROFILING macro)" OFF)
option(STC_USE_CCACHE "Use ccache for compiler caching, if it's installed locally" ON)
option(STC_USE_FORMAT "Enable formatting targets, if clang-format is installed locally" ON)
option(STC_USE_TIDY "Enable static analysis through clang-tidy, if it's installed locally" OFF)
option(STC_USE_SAN "Enable address and undefined behavior sanitizers (ASan and UBSan), if supported. Might be incompatible with some versions of Julia" OFF)
option(STC_OFFICIAL_BUILD "Mark builds produced using this configuration as an official build in the metadata" OFF)

get_property(STC_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

set(STC_BUILD_ORIGIN "development build")
if (STC_OFFICIAL_BUILD)
    set(STC_BUILD_ORIGIN "official build")
endif()
message(STATUS "Build origin: ${STC_BUILD_ORIGIN}")

if (NOT STC_MULTI_CONFIG AND "${CMAKE_BUILD_TYPE}" STREQUAL "")
    message(STATUS "No build type specified and not in multi-config environment, defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

# Julia lookup

include(${STC_SRC_DIR}/cmake/find_julia.cmake)

# C++ building begins here

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(STC_CONFIG_META_TEMPLATE "${STC_SRC_DIR}/include/meta/configure.h.in")
set(STC_CONFIG_META_OUT "${STC_BUILD_DIR}/generated/meta/configure.h")

# generate config-time metadata
configure_file("${STC_CONFIG_META_TEMPLATE}" "${STC_CONFIG_META_OUT}" @ONLY)

set(STC_BUILD_META_SCRIPT "${STC_SRC_DIR}/cmake/gen_build_meta.cmake")
set(STC_BUILD_META_TEMPLATE "${STC_SRC_DIR}/include/meta/build.h.in")
set(STC_BUILD_META_OUT "${STC_BUILD_DIR}/generated/meta/build.h")
set(STC_BUILD_META_BASE_ARGS
    "-DSTC_SRC_DIR=${STC_SRC_DIR}"
    "-DSTC_GIT_EXECUTABLE=${STC_GIT_EXECUTABLE}"
    "-DSTC_COMMIT_HASH=${STC_COMMIT_HASH}"
    "-DSTC_BUILD_TIMESTAMP=${STC_BUILD_TIMESTAMP}"
    "-DSTC_BUILD_META_TEMPLATE=${STC_BUILD_META_TEMPLATE}"
    "-DSTC_BUILD_META_OUT=${STC_BUILD_META_OUT}"
)

# generate build-time metadata, so the headers already exist before first build
# (first build overwrites this)
message(STATUS "Running build-time metadata script to populate generated headers before first build")
execute_process(
    COMMAND ${CMAKE_COMMAND} ${STC_BUILD_META_BASE_ARGS} -P "${STC_BUILD_META_SCRIPT}"
    RESULT_VARIABLE STC_BUILD_META_RESULT
)

if (NOT STC_BUILD_META_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate build metadata (exited with ${STC_BUILD_META_RESULT})")
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${STC_BUILD_META_TEMPLATE}" "${STC_BUILD_META_SCRIPT}")

set(BUILD_META_TARGET stc_build_meta)
add_custom_target(${BUILD_META_TARGET}
    COMMAND ${CMAKE_COMMAND} ${STC_BUILD_META_BASE_ARGS} -P "${STC_BUILD_META_SCRIPT}"
    BYPRODUCTS ${STC_BUILD_META_OUT}
    COMMENT "Generating build-time metadata"
    VERBATIM
)

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS "include/*.h" "src/*.cpp" "cli/*.h" "cli/*.cpp" "sandbox/*.cpp" "sandbox/*.h" "test/*.cpp" "test/*.h")

# fmt

find_package(fmt QUIET)

if(NOT fmt_FOUND)
    message(STATUS "fmt not found locally, it will be fetched through git...")
    
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        11.1.1
    )
    FetchContent_MakeAvailable(fmt)
    
    get_target_property(FMT_INCLUDES fmt INTERFACE_INCLUDE_DIRECTORIES)
    if(FMT_INCLUDES)
        set_target_properties(fmt PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${FMT_INCLUDES}")
    endif()
    
    if(TARGET fmt)
        target_compile_options(fmt PRIVATE 
            $<$<CXX_COMPILER_ID:MSVC>:/w>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
        )
    endif()
else()
    message(STATUS "fmt found locally")
endif()

# Tracy
if (STC_ENABLE_PROFILING)
    message(STATUS "STC_ENABLE_PROFILING flag provided, Tracy will be fetched through git...")
    
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    set(TRACY_LTO ON CACHE BOOL "" FORCE)
    set(TRACY_NO_SYSTEM_TRACING ON CACHE BOOL "" FORCE)
    
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG        v0.13.1
    )
    FetchContent_MakeAvailable(tracy)
endif()

# dev tooling (ccache, clang-format, clang-tidy)

include(${STC_SRC_DIR}/cmake/dev_tools.cmake)

# common compiler/linker config

add_library(CommonConfig INTERFACE)

if (MSVC)
    # apparently MSVC preprocessor is not standard-confirming by default
    # /Zc:preprocessor needed for X-macro system (__VA_ARGS__ expansion)
    target_compile_options(CommonConfig INTERFACE
        /W4 /Zc:preprocessor /GR-
        $<$<CONFIG:RelWithDebInfo>:/Zi /Oy->
    )

    target_link_options(CommonConfig INTERFACE 
        $<$<CONFIG:RelWithDebInfo>:/DEBUG>
    )
else()
    target_compile_options(CommonConfig INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion -Wreorder
        -fno-rtti
        -fvisibility=hidden
        $<$<CONFIG:RelWithDebInfo>:-g -fno-omit-frame-pointer>
    )

    target_link_options(CommonConfig INTERFACE
        $<$<CONFIG:Release>:-Wl,-s>
        $<$<CONFIG:MinSizeRel>:-Wl,-s>
    )

    if (MINGW)
        target_link_options(CommonConfig INTERFACE -static-libgcc -static-libstdc++ -static -lpthread -Wl,--allow-multiple-definition)
    endif()
endif()

if (STC_USE_SAN)
    # enable sanitizers for debug builds (and force disable incompatible options on MSVC)
    if (MSVC)
        target_compile_options(CommonConfig INTERFACE
            "$<$<CONFIG:Debug>:/fsanitize=address>"
            "$<$<CONFIG:Debug>:/Zi>"
        )

        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/ZI" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/ZI" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

        target_link_options(CommonConfig INTERFACE
            "$<$<CONFIG:Debug>:/INCREMENTAL:NO>"
        )

        if (STC_MULTI_CONFIG OR CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "ASan enabled for MSVC (debug builds)")
        endif()
    else()
        target_compile_options(CommonConfig INTERFACE
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
            "$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
            "$<$<CONFIG:Debug>:-g>"
        )

        target_link_options(CommonConfig INTERFACE
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
        )

        if (STC_MULTI_CONFIG OR CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "ASan and UBSan enabled for GCC/Clang/etc (debug builds)")
        endif()
    endif()
endif()

# link time optimization

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported)
if (lto_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
    message(STATUS "LTO enabled for Release and MinSizeRel builds")
endif()

# lib obj files (needed to support testing of symbols stripped from final binaries)

set(LIB_SOURCES ${ALL_SOURCES} )
list(FILTER LIB_SOURCES EXCLUDE REGEX "^${STC_SRC_DIR}/sandbox/.*")
list(FILTER LIB_SOURCES EXCLUDE REGEX "^${STC_SRC_DIR}/test/.*")
list(FILTER LIB_SOURCES EXCLUDE REGEX "^${STC_SRC_DIR}/cli/.*")

set(LIBSTC_OBJ_TARGET stc_objs)
add_library(${LIBSTC_OBJ_TARGET} OBJECT ${LIB_SOURCES})
add_dependencies(${LIBSTC_OBJ_TARGET} ${BUILD_META_TARGET})

target_include_directories(${LIBSTC_OBJ_TARGET} PUBLIC  ${STC_SRC_DIR}/include)
target_include_directories(${LIBSTC_OBJ_TARGET} PUBLIC  ${STC_BUILD_DIR}/generated)

target_link_libraries(${LIBSTC_OBJ_TARGET} PUBLIC CommonConfig STC::Julia fmt::fmt)
target_compile_definitions(${LIBSTC_OBJ_TARGET} PUBLIC STC_BUILDING_DLL=1 JULIA_ENABLE_THREADING=1)

if (STC_ENABLE_PROFILING)
    target_link_libraries(${LIBSTC_OBJ_TARGET} PUBLIC Tracy::TracyClient)
    target_compile_definitions(${LIBSTC_OBJ_TARGET} PUBLIC TRACY_ENABLE=1 STC_PROFILING=1)
endif()

# libstc

set(LIBSTC_TARGET stc)
add_library(${LIBSTC_TARGET} SHARED $<TARGET_OBJECTS:${LIBSTC_OBJ_TARGET}>)
target_link_libraries(${LIBSTC_TARGET} PRIVATE ${LIBSTC_OBJ_TARGET})

if (STC_ENABLE_PROFILING)
    target_link_libraries(${LIBSTC_TARGET} PRIVATE Tracy::TracyClient)    
endif()

set_target_properties(${LIBSTC_TARGET} PROPERTIES
    VERSION                                 "${PROJECT_VERSION}"
    SOVERSION                               "${STC_SOVERSION}"

    CXX_VISIBILITY_PRESET                   hidden
    C_VISIBILITY_PRESET                     hidden
    VISIBILITY_INLINES_HIDDEN               ON

    LIBRARY_OUTPUT_DIRECTORY                "${STC_LIB_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY                "${STC_LIB_DIR}"

    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${STC_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${STC_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${STC_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${STC_LIB_DIR}"

    ARCHIVE_OUTPUT_DIRECTORY_DEBUG          "${STC_LIB_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE        "${STC_LIB_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${STC_LIB_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL     "${STC_LIB_DIR}"
)

if (STC_USE_TIDY AND DEFINED CLANG_TIDY_CMD)
    set_target_properties(${LIBSTC_TARGET} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_CMD}")
endif()

# CLI

set(CLI_TARGET stc_cli)
if (STC_BUILD_CLI)
    add_executable(${CLI_TARGET}
        "cli/cli.cpp"
        "src/common/term_utils.cpp"
        "src/common/utils.cpp"
    )
    add_dependencies(${CLI_TARGET} ${BUILD_META_TARGET})

    set_target_properties(${CLI_TARGET} PROPERTIES OUTPUT_NAME "stc"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${STC_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${STC_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${STC_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${STC_BIN_DIR}"
    )

    target_link_libraries(${CLI_TARGET} PRIVATE ${LIBSTC_TARGET} CommonConfig STC::Julia)
    target_link_libraries(${CLI_TARGET} PRIVATE fmt::fmt)

    target_include_directories(${CLI_TARGET} PRIVATE ${STC_SRC_DIR}/include)
    target_include_directories(${CLI_TARGET} PRIVATE ${STC_SRC_DIR}/cli)
    target_include_directories(${CLI_TARGET} PRIVATE ${STC_BUILD_DIR}/generated)

    if (STC_USE_TIDY AND DEFINED CLANG_TIDY_CMD)
        set_target_properties(${CLI_TARGET} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_CMD}")
    endif()

    if (STC_ENABLE_PROFILING)
        target_compile_definitions(${CLI_TARGET} PRIVATE STC_PROFILING=1)
    endif()

    if (WIN32)
        add_custom_command(TARGET ${CLI_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${LIBSTC_TARGET}>
            $<TARGET_FILE_DIR:${CLI_TARGET}>
        )
    endif()
endif()

# Catch2 / testing

set(UNIT_TESTS_TARGET unit_tests)
if (STC_BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.12.0
    )

    FetchContent_MakeAvailable(Catch2)

    if (NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR "Catch2 could not be fetched, but testing has been explicitly requested.")
    endif()

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "test/*.cpp")

    add_executable(${UNIT_TESTS_TARGET} ${TEST_SOURCES})
    add_dependencies(${UNIT_TESTS_TARGET} ${BUILD_META_TARGET})

    set_target_properties(${UNIT_TESTS_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${STC_TEST_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${STC_TEST_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${STC_TEST_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${STC_TEST_DIR}"
    )

    target_compile_definitions(${UNIT_TESTS_TARGET} PRIVATE PROJECT_ROOT="${STC_SRC_DIR}")

    target_link_libraries(${UNIT_TESTS_TARGET} PRIVATE ${LIBSTC_OBJ_TARGET})
    target_link_libraries(${UNIT_TESTS_TARGET} PRIVATE Catch2::Catch2WithMain)

    if (STC_ENABLE_PROFILING)
        target_link_libraries(${UNIT_TESTS_TARGET} PRIVATE Tracy::TracyClient)
        target_compile_definitions(${UNIT_TESTS_TARGET} PRIVATE STC_PROFILING=1)
    endif()

    # copy lib for test runner
    if (WIN32)
        add_custom_command(TARGET ${UNIT_TESTS_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${LIBSTC_TARGET}>
            $<TARGET_FILE_DIR:${UNIT_TESTS_TARGET}>
        )
    endif()

    enable_testing()
    include(Catch)
    catch_discover_tests(${UNIT_TESTS_TARGET})

    message(STATUS "Catch2 found, ${UNIT_TESTS_TARGET} target enabled for testing")
else()
    message(STATUS "Skipping Catch2 build and ${UNIT_TESTS_TARGET} target")
endif()

# Doxygen (+ Graphviz)

if (STC_BUILD_DOCS)
    include(${STC_SRC_DIR}/cmake/docs.cmake)
else()
    message(STATUS "docs building is disabled")
endif()

# sandbox

if (STC_BUILD_SANDBOX)
    add_subdirectory(sandbox)
else()
    message(STATUS "sandbox building is disabled")
endif()

# install lib, cli and pkg api headers

include(GNUInstallDirs)

install(TARGETS ${LIBSTC_TARGET}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(STC_BUILD_CLI)
    install(TARGETS ${CLI_TARGET} 
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(DIRECTORY ${STC_SRC_DIR}/include/api/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/stc
    FILES_MATCHING PATTERN "*.h"
)
