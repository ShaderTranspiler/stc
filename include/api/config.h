#pragma once

#include "api/core.h"
#include "base.h"

#include <cstdint>

/// @note For a description of each configuration option, see the docs of @b TranspilerConfig.
/// @see stc::TranspilerConfig
namespace stc::api {

// C-level API for handling configuration
extern "C" {
    /// Creates a configuration handler, which identifies a unique configuration. It can be passed
    /// to setter functions to modify its options, then passed to an @c stc_transpile call to use
    /// those options during transpilation. It is the caller's responsibility to free a successfully
    /// created handler with @c stc_free_cfg.
    /// @see stc::api::stc_transpile
    /// @see stc::api::stc_free_cfg
    STC_API STC_ConfigHandle stc_create_cfg() noexcept;
    /// Frees the @a cfg_handle's underlying memory resource. If @a cfg_handle is
    /// @c STC_NULL_CONFIG, it is a no-op.
    STC_API void stc_free_cfg(STC_ConfigHandle cfg_handle) noexcept;

    /// @see stc::DumpVerbosity
    STC_API void stc_set_error_dump_verbosity(STC_ConfigHandle cfg_handle,
                                              STC_DumpVerbosityValue value) noexcept;

    // clang-format off
    STC_API void stc_set_code_gen_indent(STC_ConfigHandle cfg_handle, uint16_t value) noexcept;
    STC_API void stc_set_dump_indent(STC_ConfigHandle cfg_handle, uint16_t value) noexcept;
    STC_API void stc_set_use_tabs(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_dump_scopes(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_forward_fns(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_warn_on_jl_sema_query(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_print_convert_fail_reason(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_track_bindings(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_coerce_to_f32(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_coerce_to_i32(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_capture_uniforms(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_dump_parsed(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_dump_sema(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_dump_lowered(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_target_version(STC_ConfigHandle cfg_handle, const char* value) noexcept;
    STC_API void stc_set_local_size(STC_ConfigHandle cfg_handle, uint32_t x, uint32_t y, uint32_t z) noexcept;

    STC_API void stc_set_julia_glm_path(STC_ConfigHandle cfg_handle, const char* value) noexcept;
    STC_API void stc_set_fallback_file_path(STC_ConfigHandle cfg_handle, const char* value) noexcept;
    STC_API void stc_set_track_timing(STC_ConfigHandle cfg_handle, bool value) noexcept;
    STC_API void stc_set_track_memory(STC_ConfigHandle cfg_handle, bool value) noexcept;

    STC_API void stc_set_initial_julia_node_pool_size(STC_ConfigHandle cfg_handle, uint32_t value) noexcept;
    STC_API void stc_set_initial_sir_node_pool_size(STC_ConfigHandle cfg_handle, uint32_t value) noexcept;
    STC_API void stc_set_initial_symbol_pool_size(STC_ConfigHandle cfg_handle, uint32_t value) noexcept;
    STC_API void stc_set_initial_source_info_pool_size(STC_ConfigHandle cfg_handle, uint32_t value) noexcept;
    STC_API void stc_set_initial_type_pool_size(STC_ConfigHandle cfg_handle, uint16_t value) noexcept;
    STC_API void stc_set_initial_qualifier_pool_size(STC_ConfigHandle cfg_handle, uint16_t value) noexcept;

    /// Sets each initial pool size to its default value
    STC_API void stc_reset_initial_sizes(STC_ConfigHandle cfg_handle) noexcept;

    /// Bit faster than calling each @c stc_set_initial_*_pool_size() variant individually, since it only does config handle resolution once
    STC_API void stc_set_initial_pool_sizes(STC_ConfigHandle cfg_handle, uint32_t julia_node_pool, uint32_t sir_node_pool,
                                            uint32_t symbol_pool, uint32_t source_info_pool, uint16_t type_pool,
                                            uint16_t qualifier_pool) noexcept;
    // clang-format on
}

} // namespace stc::api
