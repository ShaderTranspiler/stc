#pragma once

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "common/id_types.hpp"

namespace stc {

/// @addtogroup api
/// @{

/// @brief Specifies what level of information is dumped along the basic source location info and
/// error message when an error occurs during transpilation.
enum class DumpVerbosity : uint8_t {
    FIRST = 0,
    None  = FIRST, ///< Nothing will be dumped when an error is encountered
    Partial,       ///< The AST subtree that caused the error will be dumped
    Verbose,       ///< The entire current AST is dumped along every error
    LAST = Verbose,
    COUNT,
};

/// Stores the amount of memory initially reserved for each pool type's internal arena
/// For most use cases, the default values should be more than enough without excessive memory usage
/// on modern systems. All values defined as bytes.
/// The two main use cases for tinkering with this are:
/// a) optimizing memory usage. if running with track_memory shows that a given transpilation
/// allocates extra slabs, and that transpilation is expected to happen repeatedly, reserving its
/// needed memory up front should speed up transpilation somewhat, since it reduces the number of
/// allocations via malloc, as well as improving cache locality (slabs are fragmented in memory)
/// b) if a single symbol is going to take up more than 2 * symbol_pool_default_kib of memory alone,
/// the allocation into the symbol pool will bad alloc, failing transpilation. This is extremely
/// unlikely to come up in any realistic scenario.
/// @important The field types match the pool IDs' internal size types. That does not guarantee that
/// setting them to the the per-type max value will not exhaust memory or the index type sooner than
/// that usage is actually hit, since arena IDs are not sequential. Setting them to the maximum
/// would allocate that much memory, and fail only when an ID value ever overruns the type's maximum
/// value when the arena tries to hand it out.
struct PoolInitSizeInfo {
    static const PoolInitSizeInfo default_sizes;

    uint32_t julia_node_pool  = 64 * 1024;
    uint32_t sir_node_pool    = 48 * 1024;
    uint32_t symbol_pool      = 4 * 1024;
    uint32_t source_info_pool = 4 * 1024;
    uint16_t type_pool        = 4 * 1024;
    uint16_t qualifier_pool   = 2 * 1024;
};
inline const PoolInitSizeInfo PoolInitSizeInfo::default_sizes{};

// i prefer to have explicitly sized types instead of aliases in PoolInitSizeInfo, since it's API
// so this is what ensures those get updated properly if ID sizes change
#define STC_STATIC_ASSERT_SAME_SIZE(field, id_t)                                                   \
    static_assert(std::same_as<decltype(std::declval<PoolInitSizeInfo>().field), id_t::id_type>)

// clang-format off
STC_STATIC_ASSERT_SAME_SIZE(julia_node_pool,  jl::NodeId);
STC_STATIC_ASSERT_SAME_SIZE(sir_node_pool,    sir::NodeId);
STC_STATIC_ASSERT_SAME_SIZE(symbol_pool,      SymbolId);
STC_STATIC_ASSERT_SAME_SIZE(source_info_pool, SrcLocationId);
STC_STATIC_ASSERT_SAME_SIZE(type_pool,        types::TypeId);
STC_STATIC_ASSERT_SAME_SIZE(qualifier_pool,   types::QualId);
// clang-format on

#undef STC_STATIC_ASSERT_SAME_SIZE

/// @brief Specifies configuration options for a transpilation pipeline. See the individual fields
/// for a list of options.
/// @important We try to validate config values, but some of it is simply not possible before
/// transpilation actually tries to use it, as an invalid value is indistinguishable from a valid
/// one. For that reason, an invalid configuration may raise an internal error instead of a code
/// error. For example, if we fail to resolve the JuliaGLM module, it's difficult to tell if it's
/// because the user provided an invalid julia_glm_path in the config, or if our module lookup logic
/// is wrong (or the libjulia logic for this changed between Julia versions). To err on the side of
/// caution, we treat this strictly and report an internal error instead of assuming the user messed
/// up, so that we don't send them on a long goose chase, looking for an issue on their side that
/// doesn't exist. However, if a config-related internal error is raised, users should check the
/// affected config options to see if it's their mistake or the transpiler's.
struct TranspilerConfig {
    static constexpr std::string_view default_julia_glm_path = "Main.JuliaGLM";
    static constexpr std::string_view default_target_version = "460";

    // General

    /// Controls the amount of memory arenas reserve initially per transpilation.
    /// For most use cases, the default values should be fine.
    /// @see PoolInitSizeInfo
    PoolInitSizeInfo initial_pool_sizes{};

    /// May specify file path that is printed as part of source location information in
    /// reports, for input code that does not carry the information on its own
    std::optional<std::string> fallback_file_path{};

    /// Specifies the lookup path of the JuliaGLM module in the current Julia instance
    std::string julia_glm_path{default_julia_glm_path};

    // GLSL
    /// The GLSL target version emitted into the generated code
    std::string target_version{default_target_version};
    uint32_t local_size_x = 0; ///< local_size_x specifier for compute shaders (set to 0 to disable)
    uint32_t local_size_y = 0; ///< local_size_y specifier for compute shaders
    uint32_t local_size_z = 0; ///< local_size_z specifier for compute shaders

    // General
    uint16_t code_gen_indent = 4; ///< The indentation size used for the generated code
    uint16_t dump_indent     = 2; ///< The indentation size used for debug dumps

    DumpVerbosity error_dump_verbosity = DumpVerbosity::None; ///< Verbosity of error dumps

    bool use_tabs = false; ///< If true, tabs are used for indentation, otherwise spaces are used.
    bool dump_scopes = false; ///< If true, scope info is dumped during sema

    /// If true, function calls that can't be resolved by us, but have a Julia-resolvable signature,
    /// will be passed further down the pipeline with their Julia return type blindly.
    /// @important May produce invalid output code.
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
    /// will be added as uniform globals in the generated code
    bool capture_uniforms = true;

    bool dump_parsed  = false; ///< If true, dumps the parsed Julia AST
    bool dump_sema    = false; ///< If true, dumps the Julia AST after semantic resolution
    bool dump_lowered = false; ///< If true, dumps the SIR AST that is created during lowering

    /// If true, per-phase timings will be recorded and printed to stdout at the end
    bool track_timing = false;

    /// If true, per-phase arena memory usage will be recorded and printed to stdout during
    /// transpilation
    bool track_memory = false;
};

/// @}

} // namespace stc
