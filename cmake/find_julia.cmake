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
#   - JULIA_RUNTIME_LIB: the absolute path of the libjulia file loaded at runtime (all platforms)
#   - JULIA_IMPORT_LIB: the absolute path of the libjulia import library (Windows only)

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

    # Julia's LIBDIR and INCLUDEDIR are specified relative to BINDIR
    execute_process(
        COMMAND ${JULIA_EXEC} -e "
            println(VERSION)
            println(Sys.BINDIR)
            println(joinpath(Sys.BINDIR, Base.LIBDIR))
            println(joinpath(Sys.BINDIR, Base.INCLUDEDIR, \"julia\"))
        "
        OUTPUT_VARIABLE FJL_JULIA_QUERY
        ERROR_VARIABLE  FJL_JULIA_QUERY_ERR
        RESULT_VARIABLE FJL_JULIA_QUERY_RES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT FJL_JULIA_QUERY_RES EQUAL 0)
        message(FATAL_ERROR "Failed to retrieve install layout from the Julia executable (${JULIA_EXEC}):\n${FJL_JULIA_QUERY_ERR}")
    endif()

    # unify newline between CRLF and LF
    string(REGEX REPLACE "\r?\n" ";" FJL_JULIA_QUERY "${FJL_JULIA_QUERY}")

    list(GET FJL_JULIA_QUERY 0 FJL_JULIA_VERSION)
    list(GET FJL_JULIA_QUERY 1 FJL_BIN_DIR_RAW)
    list(GET FJL_JULIA_QUERY 2 FJL_LIB_DIR_RAW)
    list(GET FJL_JULIA_QUERY 3 FJL_INCLUDE_DIR_RAW)

    if (DEFINED STC_JULIA_VERSION AND NOT STC_JULIA_VERSION STREQUAL "" AND NOT STC_JULIA_VERSION STREQUAL FJL_JULIA_VERSION)
        message(FATAL_ERROR "Mismatch between the explicitly specified Julia version (${STC_JULIA_VERSION}) and the Julia executable's own version info (${FJL_JULIA_VERSION}). When building with a Julia version (or install) that is separate from what the Julia executable points to, please specify all of JULIA_INCLUDE_DIR, JULIA_LIB_DIR and JULIA_BIN_DIR.")
    else()
        set(STC_JULIA_VERSION "${FJL_JULIA_VERSION}")
    endif()

    # normalize paths (Julia produces .. segments into paths and occasionally mixes \ and / on Windows)
    get_filename_component(JULIA_BIN_DIR "${FJL_BIN_DIR_RAW}" ABSOLUTE)
    get_filename_component(JULIA_LIB_DIR "${FJL_LIB_DIR_RAW}" ABSOLUTE)
    get_filename_component(JULIA_INCLUDE_DIR "${FJL_INCLUDE_DIR_RAW}" ABSOLUTE)
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

# on windows we need to locate import lib and runtime lib separately, on Unix they match
if (WIN32)
    set(JULIA_IMPORT_LIB "${JULIA_LIB_DIR}/libjulia.dll.a")

    if (NOT EXISTS "${JULIA_IMPORT_LIB}")
        message(FATAL_ERROR "Couldn't locate Julia's import library (expected path: ${JULIA_IMPORT_LIB})")
    endif()

    set(JULIA_RUNTIME_LIB "${JULIA_BIN_DIR}/libjulia.dll")
else()
    set(JULIA_RUNTIME_LIB "${JULIA_LIB_DIR}/libjulia${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

if (NOT EXISTS "${JULIA_RUNTIME_LIB}")
    message(FATAL_ERROR "Couldn't locate Julia's runtime library (expected path: ${JULIA_RUNTIME_LIB})")
endif()

message(STATUS "Using Julia include dir: ${JULIA_INCLUDE_DIR}")
message(STATUS "Using Julia bin dir: ${JULIA_BIN_DIR}")
message(STATUS "Using Julia lib dir: ${JULIA_LIB_DIR}")
message(STATUS "Using Julia runtime library: ${JULIA_RUNTIME_LIB}")
if (WIN32)
    message(STATUS "Using Julia import library: ${JULIA_IMPORT_LIB}")
endif()
message(STATUS "Using Julia version: ${STC_JULIA_VERSION}")

add_library(STC::Julia SHARED IMPORTED)
set_target_properties(STC::Julia PROPERTIES
    IMPORTED_LOCATION             "${JULIA_RUNTIME_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${JULIA_INCLUDE_DIR}"
)

# separate import lib is only used on Windows
if (WIN32)
    set_target_properties(STC::Julia PROPERTIES IMPORTED_IMPLIB "${JULIA_IMPORT_LIB}")
endif()
