# dev tools: ccache, clang-format, clang-tidy

# needs: ALL_SOURCES defined and populated before inclusion
# sets: CMAKE_{C/CXX}_COMPILER_LAUNCHER [ccache]
# sets: CLANG_TIDY_CMD [clang-tidy]
# adds targets: check_format, fix_format [clang-format]

# ccache

if (STC_USE_CCACHE)
    find_program(CCACHE_EXEC ccache)
    if (CCACHE_EXEC)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXEC}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXEC}")

        message(STATUS "ccache found and enabled for compiler caching")
    else()
        message(STATUS "ccache not found, compiler caching won't be possible")
    endif()
else()
    message(STATUS "ccache usage is disabled")
endif()

# clang-format

if (STC_USE_FORMAT)
    find_program(CLANG_FORMAT_EXEC NAMES clang-format)
    if (CLANG_FORMAT_EXEC)
        # style checking
        add_custom_target(
            "check_format"
            COMMAND ${CLANG_FORMAT_EXEC}
                    -style=file
                    -n
                    --Werror
                    ${ALL_SOURCES}
            COMMENT "Checking C++ code style..."
        )

        # automatic style enforcement
        add_custom_target(
            "fix_format"
            COMMAND ${CLANG_FORMAT_EXEC}
                    -style=file
                    -i
                    ${ALL_SOURCES}
            COMMENT "Fixing C++ code style..."
        )

        message(STATUS "clang-format found, check_format and fix_format targets enabled. Using ${CLANG_FORMAT_EXEC}")
    else()
        message(STATUS "clang-format could not be located, code style checking won't be possible.")
    endif()
else()
    message(STATUS "clang-format usage is disabled")
endif()

# clang-tidy

if (STC_USE_TIDY)
    find_program(CLANG_TIDY_EXEC clang-tidy)

    if (CLANG_TIDY_EXEC)
        set(CLANG_TIDY_CMD "${CLANG_TIDY_EXEC}")

        list(APPEND CLANG_TIDY_CMD "--extra-arg=-Wno-unknown-warning-option")
        list(APPEND CLANG_TIDY_CMD "--extra-arg=-Qunused-arguments")

        # include all header files, then exclude dependencies
        # this is to ensure new header additions are never forgotten to be added here
        # if something is vendored that shouldn't be checked, add it to the exclusion list
        list(APPEND CLANG_TIDY_CMD "--header-filter=.*")
        list(APPEND CLANG_TIDY_CMD "--exclude-header-filter=[/\\\\]_deps[/\\\\]")

        if (MSVC)
            list(APPEND CLANG_TIDY_CMD "--extra-arg-before=--driver-mode=cl")
            list(APPEND CLANG_TIDY_CMD "--extra-arg=-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH")
            list(APPEND CLANG_TIDY_CMD "--extra-arg=-Wno-unused-command-line-argument")
            list(APPEND CLANG_TIDY_CMD "--extra-arg=/EHsc")
        endif()

        message(STATUS "clang-tidy found and enabled. Using ${CLANG_TIDY_EXEC}")
    else()
        message(STATUS "clang-tidy could not be located, static analysis won't be possible.")
    endif()
else()
    message(STATUS "clang-tidy usage is disabled")
endif()
