# libjulia resolution

# locates the Julia installation to build against and exposes it to the caller via variables
# if the variables are already defined and populated, the resolution process is skipped

# ? either all explicitly specifiable variables must be provided, or none of them
# ? partial specification could result in silently incorrect builds (e.g. version mismatch between lib and headers)

# defines variables (can be explicitly specified):
#   - JULIA_INCLUDE_DIR: directory of libjulia headers
#   - JULIA_BIN_DIR: directory of julia binaries
#   - JULIA_LIB_DIR: directory of libjulia library files
#   - STC_JULIA_VERSION: the version of Julia used
# defines variables (cannot be explicitly specified):
#   - JULIA_LIB: the absolute path of the libjulia file to link against

# defines target: STC::Julia (link against libjulia through this if possible, use directories for more complex ops)

# resolution happens by querying the Julia executable directly for paths, so julia needs to be available in PATH

set(FJL_DIR_VARS JULIA_INCLUDE_DIR JULIA_LIB_DIR JULIA_BIN_DIR)

set(FJL_DIR_VARS_SPECIFIED "")
set(FJL_DIR_VARS_MISSING "")

foreach (FJL_DIR_VAR IN LISTS FJL_DIR_VARS)
    if (DEFINED ${FJL_DIR_VAR} AND NOT "${${FJL_DIR_VAR}}" STREQUAL "")
        list(APPEND FJL_DIR_VARS_SPECIFIED ${FJL_DIR_VAR})
    else()
        list(APPEND FJL_DIR_VARS_MISSING ${FJL_DIR_VAR})
    endif()
endforeach()

# all or no directory variables should be specified
if (FJL_DIR_VARS_SPECIFIED AND FJL_DIR_VARS_MISSING)
    string(REPLACE ";" ", " FJL_DIR_VARS_SPECIFIED_STR "${FJL_DIR_VARS_SPECIFIED}")
    string(REPLACE ";" ", " FJL_DIR_VARS_MISSING_STR "${FJL_DIR_VARS_MISSING}")

    message(FATAL_ERROR "Julia install paths cannot be partially specified (specified: ${FJL_DIR_VARS_SPECIFIED_STR} | missing: ${FJL_DIR_VARS_MISSING_STR}). Either pass all of JULIA_INCLUDE_DIR, JULIA_LIB_DIR and JULIA_BIN_DIR (along with STC_JULIA_VERSION), or none of them.")
endif()

# resolve through the Julia executable
if (NOT FJL_DIR_VARS_SPECIFIED)
    find_program(JULIA_EXEC julia)
    if (NOT JULIA_EXEC)
        message(FATAL_ERROR "Couldn't find Julia executable (needed for locating libjulia). Please ensure Julia is installed and added to PATH, or specify JULIA_INCLUDE_DIR, JULIA_LIB_DIR, JULIA_BIN_DIR (and STC_JULIA_VERSION) explicitly.")
    endif()

    execute_process(
        COMMAND ${JULIA_EXEC} -e "print(VERSION)"
        OUTPUT_VARIABLE STC_JULIA_VERSION_TMP
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (DEFINED STC_JULIA_VERSION AND NOT STC_JULIA_VERSION STREQUAL "" AND NOT STC_JULIA_VERSION STREQUAL STC_JULIA_VERSION_TMP)
        message(FATAL_ERROR "Mismatch between the explicitly specified Julia version (${STC_JULIA_VERSION}) and the Julia executable's own version info (${STC_JULIA_VERSION_TMP}). When building with a Julia version (or install) that is separate from what the Julia executable points to, please specify all of JULIA_INCLUDE_DIR, JULIA_LIB_DIR and JULIA_BIN_DIR.")
    else()
        set(STC_JULIA_VERSION "${STC_JULIA_VERSION_TMP}")
    endif()

    execute_process(
        COMMAND ${JULIA_EXEC} -e "print(Sys.BINDIR)"
        OUTPUT_VARIABLE JULIA_BIN_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # only used to locate the lib directory, the library file itself is resolved per-platform below
    execute_process(
        COMMAND ${JULIA_EXEC} -e "using Libdl; print(Libdl.dlpath(\"libjulia\"))"
        OUTPUT_VARIABLE FJL_JULIA_DLPATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    get_filename_component(JULIA_LIB_DIR "${FJL_JULIA_DLPATH}" DIRECTORY)
    get_filename_component(JULIA_LIB_DIR "${JULIA_LIB_DIR}" ABSOLUTE) # dlpath doesn't normalize path

    get_filename_component(JULIA_INCLUDE_DIR "${JULIA_BIN_DIR}/../include/julia" ABSOLUTE)
endif()

foreach (FJL_REQ_PATH IN LISTS FJL_DIR_VARS)
    if (NOT DEFINED ${FJL_REQ_PATH} OR "${${FJL_REQ_PATH}}" STREQUAL "")
        message(FATAL_ERROR "${FJL_REQ_PATH} not defined or empty")
    elseif (NOT EXISTS "${${FJL_REQ_PATH}}")
        message(FATAL_ERROR "Couldn't find ${FJL_REQ_PATH} in filesystem (at path: \"${${FJL_REQ_PATH}}\")")
    endif()
endforeach()

if (NOT DEFINED STC_JULIA_VERSION OR STC_JULIA_VERSION STREQUAL "")
    message(FATAL_ERROR "STC_JULIA_VERSION is required. When bypassing include/lib/bin directory resolution through the julia executable, also pass -DSTC_JULIA_VERSION=x.y.z manually.")
endif()

# CLEANUP: this is way too excessive (and some of them are redundant with the new resolution system)
if (WIN32)
    if (EXISTS "${JULIA_LIB_DIR}/libjulia.dll.a")
        set(JULIA_LIB "${JULIA_LIB_DIR}/libjulia.dll.a")
    elseif (EXISTS "${JULIA_BIN_DIR}/libjulia.dll")
        set(JULIA_LIB "${JULIA_BIN_DIR}/libjulia.dll")
    elseif (EXISTS "${JULIA_LIB_DIR}/libjulia.lib")
        set(JULIA_LIB "${JULIA_LIB_DIR}/libjulia.lib")
    elseif (EXISTS "${JULIA_LIB_DIR}/julia.lib")
        set(JULIA_LIB "${JULIA_LIB_DIR}/julia.lib")
    else()
        message(FATAL_ERROR "Couldn't locate Julia library (.dll.a or .lib in \"${JULIA_LIB_DIR}\", or .dll in \"${JULIA_BIN_DIR}\")")
    endif()
else()
    if (EXISTS "${JULIA_LIB_DIR}/libjulia.so")
        set(JULIA_LIB "${JULIA_LIB_DIR}/libjulia.so")
    elseif (EXISTS "${JULIA_LIB_DIR}/libjulia.dylib")
        set(JULIA_LIB "${JULIA_LIB_DIR}/libjulia.dylib")
    else()
        file(GLOB JULIA_LIB_MATCHES "${JULIA_LIB_DIR}/libjulia.*")
        if (JULIA_LIB_MATCHES)
            list(GET JULIA_LIB_MATCHES 0 JULIA_LIB)
        endif()
    endif()
endif()

if (NOT EXISTS "${JULIA_LIB}")
    message(FATAL_ERROR "Couldn't locate libjulia (searched in \"${JULIA_LIB_DIR}\")")
endif()

message(STATUS "Using Julia include dir: ${JULIA_INCLUDE_DIR}")
message(STATUS "Using Julia bin dir: ${JULIA_BIN_DIR}")
message(STATUS "Using Julia lib dir: ${JULIA_LIB_DIR}")
message(STATUS "Using Julia library: ${JULIA_LIB}")
message(STATUS "Using Julia version: ${STC_JULIA_VERSION}")

add_library(STC::Julia UNKNOWN IMPORTED)
set_target_properties(STC::Julia PROPERTIES
    IMPORTED_LOCATION             "${JULIA_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${JULIA_INCLUDE_DIR}"
)
