#include "stc/stc.h"

#include "common/config.hpp"

#define STC_SET_CFG_VALUE(handle, field, value)                                                    \
    if ((handle) != STC_NULL_CONFIG) {                                                             \
        static_cast<TranspilerConfig*>(handle)->field = (value);                                   \
    }

namespace stc::api {

extern "C" {
    STC_API STC_ConfigHandle stc_create_cfg() noexcept {
        try {
            return new TranspilerConfig{};
        } catch (...) {
            return STC_NULL_CONFIG;
        }
    }

    STC_API void stc_free_cfg(STC_ConfigHandle cfg_handle) noexcept {
        if (cfg_handle == STC_NULL_CONFIG)
            return;

        delete static_cast<TranspilerConfig*>(cfg_handle);
    }

    STC_API void stc_set_code_gen_indent(STC_ConfigHandle cfg_handle, uint16_t value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, code_gen_indent, value);
    }

    STC_API void stc_set_dump_indent(STC_ConfigHandle cfg_handle, uint16_t value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_indent, value);
    }

    STC_API void stc_set_error_dump_verbosity(STC_ConfigHandle cfg_handle,
                                              STC_DumpVerbosity value) noexcept {
        if (value < static_cast<STC_DumpVerbosity>(DumpVerbosity::FIRST) ||
            value > static_cast<STC_DumpVerbosity>(DumpVerbosity::LAST))
            return;

        STC_SET_CFG_VALUE(cfg_handle, error_dump_verbosity, static_cast<DumpVerbosity>(value));
    }

    STC_API void stc_set_use_tabs(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, use_tabs, value);
    }

    STC_API void stc_set_dump_scopes(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_scopes, value);
    }

    STC_API void stc_set_forward_fns(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, forward_fns, value);
    }

    STC_API void stc_set_warn_on_jl_sema_query(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, warn_on_jl_sema_query, value);
    }

    STC_API void stc_set_print_convert_fail_reason(STC_ConfigHandle cfg_handle,
                                                   bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, print_convert_fail_reason, value);
    }

    STC_API void stc_set_track_bindings(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, track_bindings, value);
    }

    STC_API void stc_set_coerce_to_f32(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, coerce_to_f32, value);
    }

    STC_API void stc_set_coerce_to_i32(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, coerce_to_i32, value);
    }

    STC_API void stc_set_capture_uniforms(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, capture_uniforms, value);
    }

    STC_API void stc_set_dump_parsed(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_parsed, value);
    }

    STC_API void stc_set_dump_sema(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_sema, value);
    }

    STC_API void stc_set_dump_lowered(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_lowered, value);
    }

    STC_API void stc_set_target_version(STC_ConfigHandle cfg_handle, const char* value) noexcept {
        if (cfg_handle == STC_NULL_CONFIG)
            return;

        auto& config = *static_cast<TranspilerConfig*>(cfg_handle);

        try {
            config.target_version =
                value != nullptr ? value : TranspilerConfig::default_target_version;
        } catch (...) {}
    }

    STC_API void stc_set_local_size_x(STC_ConfigHandle cfg_handle, uint32_t x) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, local_size_x, x);
    }

    STC_API void stc_set_local_size_y(STC_ConfigHandle cfg_handle, uint32_t y) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, local_size_y, y);
    }

    STC_API void stc_set_local_size_z(STC_ConfigHandle cfg_handle, uint32_t z) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, local_size_z, z);
    }

    STC_API void stc_set_local_size(STC_ConfigHandle cfg_handle, uint32_t x, uint32_t y,
                                    uint32_t z) noexcept {
        if (cfg_handle == STC_NULL_CONFIG)
            return;

        auto& conf        = *static_cast<TranspilerConfig*>(cfg_handle);
        conf.local_size_x = x;
        conf.local_size_y = y;
        conf.local_size_z = z;
    }

    STC_API void stc_set_julia_glm_path(STC_ConfigHandle cfg_handle, const char* value) noexcept {
        if (cfg_handle == STC_NULL_CONFIG)
            return;

        auto& config = *static_cast<TranspilerConfig*>(cfg_handle);
        try {
            if (value != nullptr)
                config.julia_glm_path = std::string{value};
            else
                config.julia_glm_path = TranspilerConfig::default_julia_glm_path;
        } catch (...) {}
    }

    STC_API void stc_set_fallback_file_path(STC_ConfigHandle cfg_handle,
                                            const char* value) noexcept {
        if (cfg_handle == STC_NULL_CONFIG)
            return;

        auto& config = *static_cast<TranspilerConfig*>(cfg_handle);
        try {
            if (value != nullptr)
                config.fallback_file_path.emplace(value);
            else
                config.fallback_file_path = std::nullopt;
        } catch (...) {}
    }

    STC_API void stc_set_track_timing(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, track_timing, value);
    }

    STC_API void stc_set_track_memory(STC_ConfigHandle cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, track_memory, value);
    }

#define STC_SET_SIZE_OR_DEFAULT(cfg_handle, field, value)                                          \
    if ((cfg_handle) != STC_NULL_CONFIG) {                                                         \
        static_cast<TranspilerConfig*>(cfg_handle)->initial_pool_sizes.field =                     \
            (value) != 0 ? (value) : PoolInitSizeInfo::default_sizes.field;                        \
    }

    STC_API void stc_set_initial_julia_node_pool_size(STC_ConfigHandle cfg_handle,
                                                      uint32_t value) noexcept {
        STC_SET_SIZE_OR_DEFAULT(cfg_handle, julia_node_pool, value);
    }

    STC_API void stc_set_initial_sir_node_pool_size(STC_ConfigHandle cfg_handle,
                                                    uint32_t value) noexcept {
        STC_SET_SIZE_OR_DEFAULT(cfg_handle, sir_node_pool, value);
    }

    STC_API void stc_set_initial_symbol_pool_size(STC_ConfigHandle cfg_handle,
                                                  uint32_t value) noexcept {
        STC_SET_SIZE_OR_DEFAULT(cfg_handle, symbol_pool, value);
    }

    STC_API void stc_set_initial_source_info_pool_size(STC_ConfigHandle cfg_handle,
                                                       uint32_t value) noexcept {
        STC_SET_SIZE_OR_DEFAULT(cfg_handle, source_info_pool, value);
    }

    STC_API void stc_set_initial_type_pool_size(STC_ConfigHandle cfg_handle,
                                                uint16_t value) noexcept {
        STC_SET_SIZE_OR_DEFAULT(cfg_handle, type_pool, value);
    }

    STC_API void stc_set_initial_qualifier_pool_size(STC_ConfigHandle cfg_handle,
                                                     uint16_t value) noexcept {
        STC_SET_SIZE_OR_DEFAULT(cfg_handle, qualifier_pool, value);
    }

    STC_API void stc_reset_initial_sizes(STC_ConfigHandle cfg_handle) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, initial_pool_sizes, PoolInitSizeInfo{});
    }

    STC_API void stc_set_initial_pool_sizes(STC_ConfigHandle cfg_handle, uint32_t julia_node_pool,
                                            uint32_t sir_node_pool, uint32_t symbol_pool,
                                            uint32_t source_info_pool, uint16_t type_pool,
                                            uint16_t qualifier_pool) noexcept {
        if (cfg_handle == STC_NULL_CONFIG)
            return;

        auto& config = *static_cast<stc::TranspilerConfig*>(cfg_handle);

#define VALUE_OR_DEFAULT(value) (value) != 0 ? (value) : PoolInitSizeInfo::default_sizes.value

        config.initial_pool_sizes = {
            .julia_node_pool  = VALUE_OR_DEFAULT(julia_node_pool),
            .sir_node_pool    = VALUE_OR_DEFAULT(sir_node_pool),
            .symbol_pool      = VALUE_OR_DEFAULT(symbol_pool),
            .source_info_pool = VALUE_OR_DEFAULT(source_info_pool),
            .type_pool        = VALUE_OR_DEFAULT(type_pool),
            .qualifier_pool   = VALUE_OR_DEFAULT(qualifier_pool),
        };

#undef VALUE_OR_DEFAULT
    }
}

} // namespace stc::api

#undef STC_SET_CFG_VALUE
