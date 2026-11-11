#pragma once

#include "api/core.h"
#include "common/config.h"
#include "common/trackers.h"
#include "frontend/jl/context.h"

#include <string>
#include <string_view>

// NOTE: we're not aiming for C compatibility in terms of using the C API straight from a C program.
// why we need the C ABI is so that Julia can access these end points through ccall-s. that's why
// it does not get its own header, or why it uses 'using' over 'typedef' and other C++ specific
// things.

namespace stc::api {

/// @addtogroup api
/// @{

// ===========
//   C++ API
// ===========

/// @brief Transpilation result returned by the C++ API.
struct TranspilationResult {
    /// The generated code if transpilation succeeded, or an empty string
    std::string code;
    /// Indicates the result status of the transpilation
    STC_Status status;

    /// A copy of this is returned upon transpilation that fails due to a code error
    static const TranspilationResult failed_result;
    /// A copy of this is returned upon transpilation that fails due to internal error(s)
    static const TranspilationResult internal_error_result;
};

inline const TranspilationResult TranspilationResult::failed_result{
    .code{},
    .status = STC_Status::Failed,
};

inline const TranspilationResult TranspilationResult::internal_error_result{
    .code{},
    .status = STC_Status::InternalError,
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

// ===========
//    C API
// ===========

// NOTE: we don't expose a C API endpoint for starting transpilation from our own Julia AST
// format, since it is a CPP-only type. The C API is meant for Julia-side invocation anyway, where
// either a String or an Expr is expected as the natural input type.

extern "C" {
    /// @brief Queries the current ABI version of the library. Note that this offers no guarantees
    /// while it's zero, regular semver guarantees begin when it reaches 1.
    /// @return The current ABI version number
    STC_API uint8_t stc_abi_version() noexcept;

    // guard for stc_transpile* endpoints return type
    // used alongside type alias because an ABI breaking change like this should be deliberate
    static_assert(std::same_as<std::underlying_type_t<STC_Status>, uint32_t>,
                  "STC_Status underlying type drifted without the C API getting updated");

    /// @brief Invokes the Julia -> GLSL transpilation pipeline on a Julia-parsed @b Expr AST
    /// @param expr_v The @b Expr representation of the source code to transpile
    /// @param cfg_handle The @b STC_ConfigHandle identifying the configuration to use during
    /// transpilation (pass @c STC_NULL_CONFIG to use the default config options)
    /// @param result_handle An @b STC_ResultHandle pointer, through which a result handle gets set
    /// to the @b STC_ResultHandle identifying the generated GLSL code upon successful
    /// transpilation, or to @c STC_NULL_RESULT if transpilation failed for any reason. Calling
    /// stc_free_result is the caller's responsibility if it's not @c STC_NULL_RESULT after
    /// transpilation (or equivalently, if the return value is @c STC_SUCCESS).
    /// @return An @b STC_Status value describing the outcome of transpilation (also see the
    /// @a result_handle parameter)
    /// @see stc::api::STC_Result
    /// @see stc::api::stc_get_result
    /// @see stc::api::stc_free_result
    /// @see stc::api::stc_create_cfg
    STC_API STC_StatusValue stc_transpile(jl_value_t* expr_v, STC_ConfigHandle cfg_handle,
                                          STC_ResultHandle* result_handle) noexcept;

    /// @brief Invokes the Julia -> GLSL transpilation pipeline on raw Julia source code
    /// @param code The raw string form of the source code to transpile.
    /// @param cfg_handle The @b STC_ConfigHandle identifying the configuration to use during
    /// transpilation (pass @c STC_NULL_CONFIG to use the default config options)
    /// @param result_handle An @b STC_ResultHandle pointer, through which a result handle gets set
    /// to the @b STC_ResultHandle identifying the generated GLSL code upon successful
    /// transpilation, or to @c STC_NULL_RESULT if transpilation failed for any reason. Calling
    /// stc_free_result is the caller's responsibility if it's not @c STC_NULL_RESULT after
    /// transpilation (or equivalently, if the return value is @c STC_SUCCESS).
    /// @return An @b STC_Status value describing the outcome of transpilation (also see the
    /// @a result_handle parameter)
    /// @see stc::api::STC_Result
    /// @see stc::api::stc_get_result
    /// @see stc::api::stc_free_result
    /// @see stc::api::stc_create_cfg
    STC_API STC_StatusValue stc_transpile_code(const char* code, STC_ConfigHandle cfg_handle,
                                               STC_ResultHandle* result_handle) noexcept;

    /// @brief Retrieves the C string representation of the generated code belonging to a result
    /// handle
    /// @param result_handle The @b STC_ResultHandle obtained through a successful transpilation via
    /// the C API.
    /// @return A pointer to the beginning of a null-terminated string of the generated GLSL code,
    /// or @c NULL if @c STC_NULL_RESULT was passed as the handle.
    /// @see stc::api::stc_free_result
    /// @see stc::api::stc_transpile
    STC_API const char* stc_get_result(STC_ResultHandle result_handle) noexcept;

    /// @brief Frees the underlying resource of a result handle.
    /// Should be called exactly once for every successful transpilation via the C API, only after
    /// the last usage of the result handle.
    /// @param result_handle The @b STC_ResultHandle obtained through a successful transpilation via
    /// the C API. Passing @c STC_NULL_RESULT is a no-op.
    /// @see stc::api::stc_transpile
    STC_API void stc_free_result(STC_ResultHandle result_handle) noexcept;
}

/// @}

}; // namespace stc::api
