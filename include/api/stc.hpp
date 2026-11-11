#pragma once

/// @file
/// @brief The C++ API of stc

#include "api/stc.h"

#include "common/config.hpp"
#include "common/trackers.hpp"
#include "frontend/jl/context.hpp"

#include <string>
#include <string_view>

namespace stc::api {

/// @addtogroup api
/// @{

/// @brief Describes the possible result states of a transpilation.
/// The C API exports these as variables, so that their aliases can be initialized dynamically on
/// the Julia-side.
/// @see stc::api::stc_transpile
enum class Status : uint8_t {
    FIRST   = 0,
    Success = FIRST, ///< Transpilation succeeded
    Failed,          ///< Transpilation failed due to errors in the source
    InternalError,   ///< Transpilation failed due to internal errors
    LAST = InternalError,
    COUNT,
};

/// @brief Transpilation result returned by the C++ API.
struct TranspilationResult {
    /// The generated code if transpilation succeeded, or an empty string
    std::string code;
    /// Indicates the result status of the transpilation
    Status status;

    /// A copy of this is returned upon transpilation that fails due to a code error
    static const TranspilationResult failed_result;
    /// A copy of this is returned upon transpilation that fails due to internal error(s)
    static const TranspilationResult internal_error_result;
};

inline const TranspilationResult TranspilationResult::failed_result{
    .code{},
    .status = Status::Failed,
};

inline const TranspilationResult TranspilationResult::internal_error_result{
    .code{},
    .status = Status::InternalError,
};

/// @brief Invokes the Julia -> GLSL transpilation pipeline on an already parsed Julia AST
/// @param jl_ast Root node of the parsed Julia AST to transpile
/// @param jl_ctx The Julia context used during the parsing of @a jl_ast
/// @param timing_tracker The @b TimingTracker to record timing information into, or @c nullptr if a
/// fresh one should be created, based on the configuration's @a track_timing field. When passing a
/// tracker explicitly, it must not be in recording state (i.e. @c start_timing called without a
/// matching call to @c end_timing).
/// @param memory_tracker The @b MemoryTracker to use for arena memory usage recording, or
/// @c nullptr if a fresh one should be created, based on the configuration's @a track_memory field.
/// @return A @b TranspilationResult describing the outcome of the transpilation attempt. If it was
/// successful, the @a status field is set to @c Success, otherwise it's either @c Failed (if the
/// code contained errors), or @c InternalError (if an internal error occurred in the transpiler).
/// Its @a code field is set to the generated GLSL code if <tt>status == Success</tt>, otherwise
/// it's an empty string.
/// @important Partial moves @a jl_ctx, meaning while its node pool remains technically usable after
/// a call to this function, all of its shared pools will have been destroyed, after they've been
/// reused by the backend context. This means all IDs in the AST pointing into shared pools get
/// invalidated. For safety, it should be treaten as if @a jl_ctx had been moved all together, and
/// not used after a call to this function.
/// @important If @a memory_tracker is passed explicitly, and it already has the resources of
/// @a jl_ctx registered, all of its shared pools will be unregistered by the time the function
/// returns. Since the node pool itself is kept alive, it remains registered by @a memory_tracker
/// and it is the caller's responsibility to unregister before @a jl_ctx is destroyed. If the
/// resources of @a jl_ctx have not been registered into @a memory_tracker prior to this call, they
/// will not be used for tracking until the @b JLCtx is partial moved, and the function takes
/// explicit ownership of its resources.
/// @see stc::api::TranspilationResult
/// @see stc::TimingTracker
/// @see stc::MemoryTracker
STC_API TranspilationResult transpile(jl::NodeId jl_ast, jl::JLCtx& jl_ctx,
                                      TimingTracker* timing_tracker = nullptr,
                                      MemoryTracker* memory_tracker = nullptr) noexcept;

/// @overload
/// @brief Invokes the Julia -> GLSL transpilation pipeline on a Julia-parsed @b Expr AST
/// @param expr_v The Julia-parsed @b Expr representation of the source code to transpile
/// @param config The @b TranspilerConfig that describes the configuration options to use during
/// transpilation
/// @important If @a memory_tracker is passed explicitly, it will have the exact same set of arenas
/// registered upon return as it had before the call.
/// @see stc::TranspilerConfig
STC_API TranspilationResult transpile(jl_value_t* expr_v, const stc::TranspilerConfig& config,
                                      TimingTracker* timing_tracker = nullptr,
                                      MemoryTracker* memory_tracker = nullptr) noexcept;

/// @overload
/// @brief Invokes the Julia -> GLSL transpilation pipeline on raw Julia source code.
/// @param code The raw string representation of the Julia source code to transpile. The code must
/// represent a compound expression (Julia <tt>begin ... end</tt> block), which is treated as the
/// top level of the source code.
/// @param config The @b TranspilerConfig that describes the configuration options to use during
/// transpilation
/// @important If @a memory_tracker is passed explicitly, it will have the exact same set of arenas
/// registered upon return as it had before the call.
/// @see stc::TranspilerConfig
STC_API TranspilationResult transpile(std::string_view code, const stc::TranspilerConfig& config,
                                      TimingTracker* timing_tracker = nullptr,
                                      MemoryTracker* memory_tracker = nullptr) noexcept;

/// @}

}; // namespace stc::api
