set(REQ_VARS STC_BUILD_META_TEMPLATE STC_BUILD_META_OUT STC_SOURCE_DIR)
set(OPT_VARS STC_BUILD_TIMESTAMP STC_COMMIT_HASH STC_GIT_EXECUTABLE)

foreach (REQ IN ITEMS ${REQ_VARS})
    if (NOT DEFINED ${REQ})
        message(FATAL_ERROR "gen_build_meta.cmake: ${REQ} is required")
    endif()
endforeach()

foreach (OPT IN ITEMS ${OPT_VARS})
    if (NOT DEFINED ${OPT})
        set(${OPT} "")
    endif()
endforeach()

if (STC_BUILD_TIMESTAMP STREQUAL "")
    string(TIMESTAMP STC_BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S %z")
endif()
message(STATUS "Using timestamp: ${STC_BUILD_TIMESTAMP}")

if (STC_COMMIT_HASH STREQUAL "")
    set(STC_COMMIT_HASH "unknown")

    if (NOT STC_GIT_EXECUTABLE STREQUAL "")
        execute_process(
            COMMAND "${STC_GIT_EXECUTABLE}" rev-parse --short HEAD
            WORKING_DIRECTORY "${STC_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_REVPARSE_OUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE GIT_REVPARSE_RESULT
            ERROR_QUIET
        )

        if (GIT_REVPARSE_RESULT EQUAL 0 AND NOT GIT_REVPARSE_OUT STREQUAL "")
            set(STC_COMMIT_HASH "${GIT_REVPARSE_OUT}")

            # flag a working tree carrying uncommitted changes to tracked files
            execute_process(
                COMMAND "${STC_GIT_EXECUTABLE}" diff --quiet HEAD --
                WORKING_DIRECTORY "${STC_SOURCE_DIR}"
                RESULT_VARIABLE GIT_DIFF_RESULT
                ERROR_QUIET
            )

            if (NOT GIT_DIFF_RESULT EQUAL 0)
                set(STC_COMMIT_HASH "${STC_COMMIT_HASH}-dirty")
            endif()
        endif()
    endif()
endif()
message(STATUS "Using commit hash: ${STC_COMMIT_HASH}")

configure_file("${STC_BUILD_META_TEMPLATE}" "${STC_BUILD_META_OUT}" @ONLY)
