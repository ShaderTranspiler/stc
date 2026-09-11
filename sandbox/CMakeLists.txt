# sandbox targets

# TODO: is the sandbox system still needed?

# for trying out and playing around with the library. cpp files prefixed with sb_ are each
# built as their own executable, with libstc linked automatically

# needs:
#   - STC_SRC_DIR, STC_BIN_DIR, STC_BUILD_DIR
#   - JULIA_LIB, JULIA_BIN_DIR, JULIA_INCLUDE_DIR
#   - LIBSTC_TARGET
#   - CommonConfig INTERFACE
#   - fmt::fmt
# respects:
#   - CLANG_TIDY_CMD / STC_USE_TIDY

set(STC_SANDBOX_BIN_DIR "${STC_BIN_DIR}/sandbox")

message(STATUS "Setting up sandbox targets...")

file(GLOB SANDBOX_SOURCES CONFIGURE_DEPENDS "${STC_SRC_DIR}/sandbox/sb_*.cpp")

foreach (SBOX_FILE ${SANDBOX_SOURCES})
    get_filename_component(FNAME ${SBOX_FILE} NAME_WE)

    add_executable(${FNAME} ${SBOX_FILE})

    set_target_properties(${FNAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${STC_SANDBOX_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${STC_SANDBOX_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${STC_SANDBOX_BIN_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${STC_SANDBOX_BIN_DIR}"
    )

    target_link_libraries(${FNAME} PRIVATE ${LIBSTC_TARGET} CommonConfig)
    target_link_libraries(${FNAME} PRIVATE STC::Julia fmt::fmt)

    target_include_directories(${FNAME} PRIVATE ${STC_SRC_DIR}/include)
    target_include_directories(${FNAME} PRIVATE ${STC_BUILD_DIR}/generated)

    if (STC_USE_TIDY AND DEFINED CLANG_TIDY_CMD)
        set_target_properties(${FNAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_CMD}")
    endif()

    # copy lib next to sandbox executable
    if (WIN32)
        add_custom_command(TARGET ${FNAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${LIBSTC_TARGET}>
            $<TARGET_FILE_DIR:${FNAME}>
        )
    endif()

    # create wrappers for executable, with Julia first in PATH
    if (WIN32)
        file(TO_NATIVE_PATH "${JULIA_BIN_DIR}" JULIA_BIN_DIR_NATIVE)
        
        file(GENERATE OUTPUT "${STC_SANDBOX_BIN_DIR}/run_${FNAME}.bat"
            CONTENT "@echo off\nset PATH=${JULIA_BIN_DIR_NATIVE};%PATH%\n\"$<TARGET_FILE:${FNAME}>\" %*"
            CONDITION $<CONFIG:${CMAKE_BUILD_TYPE}>
        )
    endif()

    message(STATUS "Created sandbox target: ${FNAME}")
endforeach()
