#pragma once

#include <cstdint>
#include <string>

namespace stc {

/// @addtogroup api
/// @{

/// @brief Specifies what level of information is dumped along the basic source location info and
/// error message
enum class DumpVerbosity : uint8_t {
    First = 0,
    None  = First, ///< Nothing will be dumped when an error is encountered
    Partial,       ///< The AST subtree that caused the error will be dumped
    Verbose,       ///< The entire current AST is dumped along every error
    Last = Verbose
};

/// @brief Specifies configuration options for a transpilation pipeline
struct TranspilerConfig {
    // GLSL
    std::string target_version = "460"; ///< The GLSL target version emitted into the generated code
    uint32_t local_size_x = 0; ///< local_size_x specifier for compute shaders (set to 0 to disable)
    uint32_t local_size_y = 0; ///< local_size_y specifier for compute shaders
    uint32_t local_size_z = 0; ///< local_size_z specifier for compute shaders

    // General
    uint16_t code_gen_indent         = 4; ///< The indentation size used for the generated code
    uint16_t dump_indent             = 2; ///< The indentation size used for debug dumps
    DumpVerbosity err_dump_verbosity = DumpVerbosity::None; ///< Verbosity for error dumps
    bool use_tabs                    = false; ///< If true, tabs are used for indentation
    bool dump_scopes                 = false; ///< If true, scope info is dumped during sema

    /// If true, fn calls with Julia-resolvable signatures will be passed further down the pipeline
    /// as-is
    bool forward_fns = false;

    /// If true, prints a warning for sema-related queries to the Julia runtime
    bool warn_on_jl_sema_query = false;

    /// If true, prints type conversion/casting failure reasons
    bool print_convert_fail_reason = false;

    /// If true, prints step-by-step resolution logic for how each symbol bindig was determined
    bool track_bindings = false;

    bool coerce_to_f32 = true; ///< If true, treats f64 literals as f32
    bool coerce_to_i32 = true; ///< If true, treats i64 literals as i32

    /// If true, globals that can be queried from Julia, but are defined outside the source code
    /// will be added as uniforms into the generated code
    bool capture_uniforms = true;

    bool dump_parsed  = false; ///< If true, dumps the parsed Julia AST
    bool dump_sema    = false; ///< If true, dumps the Julia AST after semantic resolution
    bool dump_lowered = false; ///< If true, dumps the SIR AST that is created during lowering
};

/// @}

} // namespace stc
