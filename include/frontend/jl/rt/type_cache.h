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

    explicit JuliaTypeCache(JuliaModuleCache& mod_cache) {
        auto get_from_core = [&mod_cache](const char* sym_name) {
            static jl_module_t* jl_core = mod_cache.core_mod.mod_ptr();

            auto* result = safe_cast<jl_datatype_t>(jl_get_global(jl_core, jl_symbol(sym_name)));
            if (result == nullptr)
                throw std::logic_error{std::format(
                    "failed to load datatype '{}' from Julia through the Core module", sym_name)};

            return result;
        };

        uint128 = get_from_core("UInt128");
        int128  = get_from_core("Int128");
    }

    // FEATURE: internal caching for applied types (would require smart GC rooting)
    jl_datatype_t* vector_of(jl_datatype_t* el_type) const {
        jl_value_t* vec_val = jl_apply_array_type(reinterpret_cast<jl_value_t*>(el_type), 1);
        return safe_cast<jl_datatype_t>(vec_val);
    }
};

} // namespace stc::jl::rt
