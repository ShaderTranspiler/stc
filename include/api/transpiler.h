#pragma once

#include "api/benchmark_tracker.h"
#include "common/config.h"
#include "frontend/jl/ast.h"
#include "frontend/jl/context.h"

#include <optional>
#include <string>
#include <string_view>

/// @brief Functions for handling transpilations via the library both internally and externally.
/// It exposes a set of C-side and C++-side endpoints, which handle both
///
/// @warning The C++-only templated transpile calls are only publicly exported because of the mixing
/// of the project's architecture. They're meant to be used only if the linking project is also
/// being built under identical conditions to the library (e.g. the CLI). Otherwise, prefer the C
/// API.
namespace stc::api {

/// @addtogroup api
/// @{

using MaybeString = std::optional<std::string>;

/// @brief  Invokes the Julia -> GLSL transpilation pipeline on an already parsed Julia AST
/// @tparam RunBenchmark If true, times each transpilation phase and prints results to stdout at the
/// end
/// @param jl_ast The parsed Julia AST to transpile
/// @param jl_ctx The Julia context used during parsing
/// @param benchmark_tracker The stc::detail::BenchmarkTracker previously used to time parsing and
/// init
/// @return The generated GLSL code as a std::string on success, std::nullopt on transpilation
/// failure (both for invalid code, and internal error)
/// @see stc::detail::BenchmarkTracker
template <bool RunBenchmark>
STC_API MaybeString transpile_parsed(jl::NodeId jl_ast, jl::JLCtx& jl_ctx,
                                     detail::BenchmarkTracker<RunBenchmark>& benchmark_tracker);

/// @brief Invokes the Julia -> GLSL transpilation pipeline on unparsed Julia code
/// @tparam RunBenchmark If true, times each transpilation phase and prints results at the end
/// @param code The raw string representation of the source code to transpile
/// @param file_path The optional file path that will be used for source location information
/// @param config The stc::TranspilerConfig that describes the configuration options to use during
/// transpilation
/// @param juliaglm_path Julia-side path to the JuliaGLM module
/// @return The generated GLSL code as a std::string on success, std::nullopt on transpilation
/// failure (both for invalid code, and internal error)
/// @see stc::TranspilerConfig
template <bool RunBenchmark>
STC_API MaybeString transpile(std::string_view code, std::optional<std::string_view> file_path,
                              stc::TranspilerConfig config,
                              std::string_view juliaglm_path = "Main.JuliaGLM");

/// @overload
/// @param expr_v The Julia-parsed Expr representation of the source code to transpile
template <bool RunBenchmark>
STC_API MaybeString transpile(jl_value_t* expr_v, stc::TranspilerConfig config,
                              std::string_view juliaglm_path = "Main.JuliaGLM");

extern "C" {
    /// @brief Queries the current ABI version of the library
    /// @return The current ABI version number
    STC_API uint8_t stc_abi_version() noexcept;

    /// @brief Invokes the Julia -> GLSL transpilation pipeline on Julia-parsed code
    /// @param expr_v The Expr representation of the source code to transpile
    /// @param run_benchmark If true, times each transpilation phase and prints it to stdout at the
    /// end
    /// @param cfg_handle The configuration handle to use during transpilation (set to NULL for
    /// default config options)
    /// @return A result handle pointing to the generated GLSL code on success, nullptr on failure
    /// @see stc_get_result, stc_free_result, stc_create_cfg
    STC_API void* stc_transpile(jl_value_t* expr_v, bool run_benchmark, void* cfg_handle) noexcept;

    /// @brief Retrieves the C string representation of the generated code belonging to a result
    /// handle
    /// @param result_handle The result handled obtained through a successful transpilation via the
    /// C API
    /// @return A pointer to the beginning of a null-terminated string of the generated GLSL code
    /// @see stc_free_result, stc_transpile
    STC_API const char* stc_get_result(void* result_handle) noexcept;

    /// @brief Frees the underlying resource of a result handle.
    /// Should be called exactly once for every successful transpilation via the C API, after the
    /// last usage of the result handle.
    /// @param result_handle The result handle obtained through a successful transpilation via the C
    /// API
    /// @see stc_transpile
    STC_API void stc_free_result(void* result_handle) noexcept;
}

/// @}

}; // namespace stc::api
