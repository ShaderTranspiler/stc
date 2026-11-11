#pragma once

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>

// ! This header is only meant to be included in GLSLTargetInfo's implementation
// ! This allows defining the underlying constant builtin data without exposing it into the lib

namespace stc::glsl::builtins {

#define STC_DEF_VEC_ENUMS(suffix) Vec2##suffix, Vec3##suffix, Vec4##suffix

// clang-format off
#define STC_DEF_MAT_ENUMS(suffix)                                                                  \
    Mat2x2##suffix, Mat2x3##suffix, Mat2x4##suffix,                                                \
    Mat3x2##suffix, Mat3x3##suffix, Mat3x4##suffix,                                                \
    Mat4x2##suffix, Mat4x3##suffix, Mat4x4##suffix
// clang-format on

enum class BuiltinType : uint8_t {
    Void,
    Bool,
    Int,
    UInt,
    Float,
    Double,

    STC_DEF_VEC_ENUMS(F),
    STC_DEF_VEC_ENUMS(D),
    STC_DEF_VEC_ENUMS(B),
    STC_DEF_VEC_ENUMS(I),
    STC_DEF_VEC_ENUMS(U),

    STC_DEF_MAT_ENUMS(F),
    STC_DEF_MAT_ENUMS(D),

    FirstGenericFloat,
    VecF = FirstGenericFloat,
    GenF,
    OutGenF          = GenF,
    LastGenericFloat = OutGenF,

    FirstGenericDouble,
    VecD = FirstGenericDouble,
    GenD,
    OutGenD           = GenD,
    LastGenericDouble = OutGenD,

    FirstGenericBool,
    VecB = FirstGenericBool,
    GenB,
    LastGenericBool = GenB,

    FirstGenericInt,
    VecI = FirstGenericInt,
    GenI,
    OutGenI        = GenI,
    LastGenericInt = OutGenI,

    FirstGenericUInt,
    VecU = FirstGenericUInt,
    GenU,
    OutGenU         = GenU,
    LastGenericUInt = OutGenU,

    InOutInt  = Int,
    InOutUInt = UInt,
    FirstVec  = Vec2F,
    LastVec   = Vec4U,
    FirstMat  = Mat2x2F,
    LastMat   = Mat4x4D
};

#undef STC_DEF_MAT_ENUMS
#undef STC_DEF_VEC_ENUMS

struct BuiltinFnSig {
    BuiltinType ret_ty;
    uint8_t arg_count;
    BuiltinType args[6];

    consteval BuiltinFnSig() = default;

    template <std::same_as<BuiltinType>... ArgTys>
    consteval BuiltinFnSig(BuiltinType ret_ty, ArgTys... arg_tys)
        : ret_ty{ret_ty}, arg_count{sizeof...(arg_tys)}, args{arg_tys...} {}
};

struct BuiltinFn {
    std::string_view name;
    std::span<const BuiltinFnSig> overloads;
};

extern const std::span<const BuiltinFn> builtin_fns;

inline constexpr bool is_gentype(BuiltinType ty) {
    using enum BuiltinType;

    return ty == GenF || ty == GenD || ty == GenB || ty == GenI || ty == GenU;
}

inline constexpr bool is_genvec(BuiltinType ty) {
    using enum BuiltinType;

    return ty == VecF || ty == VecD || ty == VecB || ty == VecI || ty == VecU;
}

inline constexpr bool is_generic_type(BuiltinType ty) {
    return is_gentype(ty) || is_genvec(ty);
}

inline constexpr bool is_vec(BuiltinType ty) {
    return BuiltinType::FirstVec <= ty && ty <= BuiltinType::LastVec;
}

inline constexpr bool is_mat(BuiltinType ty) {
    return BuiltinType::FirstMat <= ty && ty <= BuiltinType::LastMat;
}

inline constexpr BuiltinType el_type(BuiltinType ty) {
    using enum BuiltinType;

    if (!is_vec(ty) && !is_mat(ty))
        throw std::logic_error{"trying to call el_type on non-vector, non-matrix builtin type"};

    switch (ty) {
        case Vec2F:
        case Vec3F:
        case Vec4F:
        case Mat2x2F:
        case Mat2x3F:
        case Mat2x4F:
        case Mat3x2F:
        case Mat3x3F:
        case Mat3x4F:
        case Mat4x2F:
        case Mat4x3F:
        case Mat4x4F:
            return Float;

        case Vec2D:
        case Vec3D:
        case Vec4D:
        case Mat2x2D:
        case Mat2x3D:
        case Mat2x4D:
        case Mat3x2D:
        case Mat3x3D:
        case Mat3x4D:
        case Mat4x2D:
        case Mat4x3D:
        case Mat4x4D:
            return Double;

        case Vec2B:
        case Vec3B:
        case Vec4B:
            return Bool;

        case Vec2I:
        case Vec3I:
        case Vec4I:
            return Int;

        case Vec2U:
        case Vec3U:
        case Vec4U:
            return UInt;

        default:
            throw std::logic_error{"unaccounted collection type in el_type"};
    }
}

inline constexpr uint8_t el_count(BuiltinType ty) {
    using enum BuiltinType;

    if (!is_vec(ty))
        throw std::logic_error{"trying to call el_count on non-vector builtin type"};

    switch (ty) {
        case Vec2F:
        case Vec2D:
        case Vec2I:
        case Vec2U:
        case Vec2B:
            return 2;

        case Vec3D:
        case Vec3F:
        case Vec3I:
        case Vec3U:
        case Vec3B:
            return 3;

        case Vec4D:
        case Vec4F:
        case Vec4I:
        case Vec4U:
        case Vec4B:
            return 4;

        default:
            throw std::logic_error{"unaccounted vector type in el_count"};
    }
}

inline constexpr uint8_t col_count(BuiltinType ty) {
    using enum BuiltinType;

    if (!is_mat(ty))
        throw std::logic_error{"trying to call col_count on non-matrix builtin type"};

    switch (ty) {
        case Mat2x2F:
        case Mat2x3F:
        case Mat2x4F:
        case Mat2x2D:
        case Mat2x3D:
        case Mat2x4D:
            return 2;

        case Mat3x2F:
        case Mat3x3F:
        case Mat3x4F:
        case Mat3x2D:
        case Mat3x3D:
        case Mat3x4D:
            return 3;

        case Mat4x2F:
        case Mat4x3F:
        case Mat4x4F:
        case Mat4x2D:
        case Mat4x3D:
        case Mat4x4D:
            return 4;

        default:
            throw std::logic_error{"unaccounted matrix type in col_count"};
    }
}

inline constexpr uint8_t row_count(BuiltinType ty) {
    using enum BuiltinType;

    if (!is_mat(ty))
        throw std::logic_error{"trying to call row_count on non-matrix builtin type"};

    switch (ty) {
        case Mat2x2F:
        case Mat3x2F:
        case Mat4x2F:
        case Mat2x2D:
        case Mat3x2D:
        case Mat4x2D:
            return 2;

        case Mat2x3F:
        case Mat3x3F:
        case Mat4x3F:
        case Mat2x3D:
        case Mat3x3D:
        case Mat4x3D:
            return 3;

        case Mat2x4F:
        case Mat3x4F:
        case Mat4x4F:
        case Mat2x4D:
        case Mat3x4D:
        case Mat4x4D:
            return 4;

        default:
            throw std::logic_error{"unaccounted matrix type in row_count"};
    }
}

} // namespace stc::glsl::builtins
