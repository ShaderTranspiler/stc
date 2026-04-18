#pragma once

#include "frontend/jl/rt/module_cache.h"

namespace stc::jl::rt {

struct JuliaTypeCache {
    jl_datatype_t* bool_ = jl_bool_type;

    jl_datatype_t* int8   = jl_int8_type;
    jl_datatype_t* uint8  = jl_uint8_type;
    jl_datatype_t* int16  = jl_int16_type;
    jl_datatype_t* uint16 = jl_uint16_type;
    jl_datatype_t* int32  = jl_int32_type;
    jl_datatype_t* uint32 = jl_uint32_type;
    jl_datatype_t* int64  = jl_int64_type;
    jl_datatype_t* uint64 = jl_uint64_type;
    jl_datatype_t* int128;
    jl_datatype_t* uint128;

    jl_datatype_t* float16 = jl_float16_type;
    jl_datatype_t* float32 = jl_float32_type;
    jl_datatype_t* float64 = jl_float64_type;

    jl_unionall_t* array_ua = jl_array_type;

    jl_datatype_t* vec2;
    jl_datatype_t* vec3;
    jl_datatype_t* vec4;

    jl_unionall_t* vec_nt_ua;
    jl_unionall_t* vec_2t_ua;
    jl_unionall_t* vec_3t_ua;
    jl_unionall_t* vec_4t_ua;

    jl_unionall_t* mat_nmt_ua;

    explicit JuliaTypeCache(JuliaModuleCache& mod_cache) {
        auto get_dt_from = [](JuliaModule& mod, const char* sym_name) {
            auto* result =
                safe_cast<jl_datatype_t>(jl_get_global(mod.mod_ptr(), jl_symbol(sym_name)));
            if (result == nullptr)
                throw std::logic_error{std::format(
                    "failed to load datatype '{}' from Julia through the Core module", sym_name)};

            return result;
        };

        auto get_ua_from = [](JuliaModule& mod, const char* sym_name) {
            auto* ua_val = jl_get_global(mod.mod_ptr(), jl_symbol(sym_name));
            if (!jl_is_unionall(ua_val))
                throw std::logic_error{std::format(
                    "value for '{}' loaded from Julia is not a UnionAll type", sym_name)};

            return safe_cast<jl_unionall_t>(ua_val);
        };

        uint128 = get_dt_from(mod_cache.core_mod, "UInt128");
        int128  = get_dt_from(mod_cache.core_mod, "Int128");

        vec2 = get_dt_from(mod_cache.glm_mod, "Vec2");
        vec3 = get_dt_from(mod_cache.glm_mod, "Vec3");
        vec4 = get_dt_from(mod_cache.glm_mod, "Vec4");

        vec_nt_ua = get_ua_from(mod_cache.glm_mod, "VecNT");
        vec_2t_ua = get_ua_from(mod_cache.glm_mod, "Vec2T");
        vec_3t_ua = get_ua_from(mod_cache.glm_mod, "Vec3T");
        vec_4t_ua = get_ua_from(mod_cache.glm_mod, "Vec4T");

        mat_nmt_ua = get_ua_from(mod_cache.glm_mod, "MatNxMT");
    }

    // FEATURE: internal caching for applied types (would require smart GC rooting)
    jl_datatype_t* vector_of(jl_datatype_t* el_type) const {
        jl_value_t* vec_val = jl_apply_array_type(reinterpret_cast<jl_value_t*>(el_type), 1);
        return safe_cast<jl_datatype_t>(vec_val);
    }
};

} // namespace stc::jl::rt
