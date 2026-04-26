#pragma once

#include "base.h"
#include "julia_guard.h"
#include "types/type_pool.h"

#include <utility>

namespace stc::jl {

using namespace stc::types;

template <typename T>
struct jl_cast_trait;

template <>
struct jl_cast_trait<jl_sym_t> {
    static bool is_type_of(jl_value_t* value) { return jl_is_symbol(value); }
};

template <>
struct jl_cast_trait<jl_expr_t> {
    static bool is_type_of(jl_value_t* value) { return jl_is_expr(value); }
};

template <>
struct jl_cast_trait<jl_module_t> {
    static bool is_type_of(jl_value_t* value) { return jl_is_module(value); }
};

template <>
struct jl_cast_trait<jl_datatype_t> {
    static bool is_type_of(jl_value_t* value) { return jl_is_datatype(value); }
};

template <>
struct jl_cast_trait<jl_unionall_t> {
    static bool is_type_of(jl_value_t* value) { return jl_is_unionall(value); }
};

template <typename T>
concept CSafeCastable = requires (jl_value_t* value) {
    { jl_cast_trait<T>::is_type_of(value) } -> std::same_as<bool>;
};

// performs extra assumption checks in debug builds, same as jl_fieldref in release builds
// field_name is only used for debug assertions
[[nodiscard]]
STC_FORCE_INLINE jl_value_t* safe_fieldref(jl_value_t* node, size_t index,
                                           [[maybe_unused]] const char* field_name) {
#ifndef NDEBUG
    auto* dt         = reinterpret_cast<jl_datatype_t*>(jl_typeof(node));
    int actual_index = jl_field_index(dt, jl_symbol(field_name), 0);

    assert(actual_index >= 0 && std::cmp_equal(actual_index, index) &&
           "invalid libjulia C API assumption");
    assert(index < jl_datatype_nfields(dt) && "invalid julia C API fieldref index");
#endif

    return jl_fieldref(node, index);
}

template <typename T>
requires CSafeCastable<T>
[[nodiscard]]
STC_FORCE_INLINE T* safe_cast(jl_value_t* value) {
    if (value == nullptr)
        return nullptr;

    assert(jl_cast_trait<T>::is_type_of(value) &&
           "trying to cast jl_value_t pointer to invalid type");

    return reinterpret_cast<T*>(value);
}

template <typename T>
requires CSafeCastable<T>
[[nodiscard]]
STC_FORCE_INLINE T* try_cast(jl_value_t* value) {
    if (value == nullptr || !jl_cast_trait<T>::is_type_of(value))
        return nullptr;

    return reinterpret_cast<T*>(value);
}

[[nodiscard]]
STC_FORCE_INLINE bool check_exceptions() {
    if (jl_value_t* ex = jl_exception_occurred()) {
        jl_static_show(jl_stderr_stream(), ex);
        std::cerr << '\n';
        jl_exception_clear();
        return true;
    }

    return false;
}

[[nodiscard]]
inline std::string get_jl_fn_name(jl_value_t* fn) {
    jl_value_t* nameof_fn  = jl_get_function(jl_base_module, "nameof");
    jl_value_t* fn_sym_val = jl_call1(nameof_fn, fn);

    if (check_exceptions()) {
        std::cerr << "the above error occured during resolving the name of a function\n";
        return "?";
    }

    return jl_symbol_name(safe_cast<jl_sym_t>(fn_sym_val));
}

[[nodiscard]]
inline bool is_spec_of(jl_datatype_t* dt, jl_unionall_t* ua) {
    jl_value_t* unwrapped_ua = nullptr;
    JL_GC_PUSH1(&unwrapped_ua);
    const ScopeGuard jl_gc_pop_sg{[]() { JL_GC_POP(); }};

    unwrapped_ua = jl_unwrap_unionall(reinterpret_cast<jl_value_t*>(ua));

    jl_datatype_t* unwrapped_ua_dt = safe_cast<jl_datatype_t>(unwrapped_ua);

    return dt->name == unwrapped_ua_dt->name;
}

[[nodiscard]]
inline bool is_jl_convertible(TypeId from, TypeId to, const TypePool& type_pool) {
    if (from == to)
        return true;

    const auto& from_td = type_pool.get_td(from);
    const auto& to_td   = type_pool.get_td(to);

    if (!from_td.is_scalar() || !to_td.is_scalar())
        return false;

    // since it's not possible to catch inexact errors, like julia does, only widening conversions
    // are allowed

    if (to_td.is<IntTD>()) {
        if (from_td.is<IntTD>()) {
            IntTD to_int   = to_td.as<IntTD>();
            IntTD from_int = from_td.as<IntTD>();

            return to_int.width >= from_int.width;
        }

        if (from_td.is<BoolTD>())
            return true;

        return false;
    }

    if (to_td.is<FloatTD>()) {
        if (from_td.is<IntTD>() || from_td.is<BoolTD>())
            return true;

        if (from_td.is<FloatTD>()) {
            FloatTD to_float   = to_td.as<FloatTD>();
            FloatTD from_float = from_td.as<FloatTD>();

            return to_float.enc == from_float.enc;
        }

        return false;
    }

    return false;
}

} // namespace stc::jl
