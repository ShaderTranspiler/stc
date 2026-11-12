#include "backend/glsl/target_info.hpp"
#include "backend/glsl/builtin_data.hpp"
#include "types/type_to_string.hpp"

#include <algorithm>

namespace stc::glsl {

namespace detail {
using namespace stc::glsl::builtins;

TypeId builtin_ty_to_id(BuiltinType ty, GLSLTypes& types) {
    using enum BuiltinType;

    assert(!is_generic_type(ty) && "generic type not unrolled during builtin parsing");

    if (ty == Void)
        return types.gl_void;

    if (ty == Float)
        return types.gl_float;

    if (ty == Double)
        return types.gl_double;

    if (ty == Int)
        return types.gl_int;

    if (ty == UInt)
        return types.gl_uint;

    if (ty == Bool)
        return types.gl_bool;

    if (is_vec(ty)) {
        BuiltinType el_type_ = el_type(ty);
        TypeId el_type_id    = builtin_ty_to_id(el_type_, types);
        return types.gl_TvecN(el_type_id, el_count(ty));
    }

    if (is_mat(ty)) {
        BuiltinType el_type_ = el_type(ty);
        TypeId el_type_id    = builtin_ty_to_id(el_type_, types);

        uint8_t col_count_ = col_count(ty);
        uint8_t row_count_ = row_count(ty);

        return types.gl_TmatNxM(el_type_id, col_count_, row_count_);
    }

    throw std::logic_error{"Invalid signature in GLSL's builtin function list"};
}

} // namespace detail

TargetInfo::GlobalList GLSLTargetInfo::create_globals(GLSLTypes& types) {
    TargetInfo::GlobalList globals = {
        // clang-format off
        // VERTEX SHADER (7.1.1)
        {"gl_VertexID",             types.gl_int},
        {"gl_InstanceID",           types.gl_int},
        {"gl_DrawID",               types.gl_int},
        {"gl_BaseVertex",           types.gl_int},
        {"gl_BaseInstance",         types.gl_int},
        {"gl_Position",             types.gl_vec4},
        {"gl_PointSize",            types.gl_float},
        {"gl_ClipDistance",         types.type_pool.any_array_td(types.gl_float)},
        {"gl_CullDistance",         types.type_pool.any_array_td(types.gl_float)},

        // TESSELLATION CONTROL/EVALUATION SHADER (7.1.2-7.1.3)
        {"gl_PatchVerticesIn",      types.gl_int},
        {"gl_PrimitiveID",          types.gl_int},
        {"gl_InvocationID",         types.gl_int},
        {"gl_TessLevelOuter",       types.gl_array(types.gl_float, 4)},
        {"gl_TessLevelInner",       types.gl_array(types.gl_float, 2)},
        {"gl_TessCoord",            types.gl_vec3},

        // GEOMETRY SHADER (7.1.4)
        {"gl_PrimitiveIDIn",        types.gl_int},
        {"gl_Layer",                types.gl_int},
        {"gl_ViewportIndex",        types.gl_int},

        // FRAGMENT SHADER (7.1.5)
        {"gl_FragCoord",            types.gl_vec4},
        {"gl_FrontFacing",          types.gl_bool},
        {"gl_PointCoord",           types.gl_vec2},
        {"gl_HelperInvocation",     types.gl_bool},
        {"gl_FragDepth",            types.gl_float},
        {"gl_SampleID",             types.gl_int},
        {"gl_SamplePosition",       types.gl_vec2},
        {"gl_SampleMaskIn",         types.type_pool.any_array_td(types.gl_int)},
        {"gl_SampleMask",           types.type_pool.any_array_td(types.gl_int)},

        // COMPUTE SHADERS (7.1.6)
        {"gl_NumWorkGroups",        types.gl_uvec3},
        {"gl_WorkGroupSize",        types.gl_uvec3},
        {"gl_WorkGroupID",          types.gl_uvec3},
        {"gl_LocalInvocationID",    types.gl_uvec3},
        {"gl_GlobalInvocationID",   types.gl_uvec3},
        {"gl_LocalInvocationIndex", types.gl_uint},

        // Compatibility Profile Built-In Language Variables (7.1.7)
        {"gl_ClipVertex;",                               types.gl_vec4},
        {"vec4 gl_FrontColor",                           types.gl_vec4},
        {"vec4 gl_BackColor",                            types.gl_vec4},
        {"vec4 gl_FrontSecondaryColor",                  types.gl_vec4},
        {"vec4 gl_BackSecondaryColor",                   types.gl_vec4},
        {"vec4 gl_TexCoord",                             types.gl_vec4},
        {"float gl_FogFragCoord",                        types.gl_float},

        // Built-In Constans (7.3)
        {"gl_MaxVertexAttribs",                          types.gl_int},
        {"gl_MaxVertexUniformVectors",                   types.gl_int},
        {"gl_MaxVertexUniformComponents",                types.gl_int},
        {"gl_MaxVertexOutputComponents",                 types.gl_int},
        {"gl_MaxVaryingComponents",                      types.gl_int},
        {"gl_MaxVaryingVectors",                         types.gl_int},
        {"gl_MaxVertexTextureImageUnits",                types.gl_int},
        {"gl_MaxVertexImageUniforms",                    types.gl_int},
        {"gl_MaxVertexAtomicCounters",                   types.gl_int},
        {"gl_MaxVertexAtomicCounterBuffers",             types.gl_int},
        {"gl_MaxTessPatchComponents",                    types.gl_int},
        {"gl_MaxPatchVertices",                          types.gl_int},
        {"gl_MaxTessGenLevel",                           types.gl_int},
        {"gl_MaxTessControlInputComponents",             types.gl_int},
        {"gl_MaxTessControlOutputComponents",            types.gl_int},
        {"gl_MaxTessControlTextureImageUnits",           types.gl_int},
        {"gl_MaxTessControlUniformComponents",           types.gl_int},
        {"gl_MaxTessControlTotalOutputComponents",       types.gl_int},
        {"gl_MaxTessControlImageUniforms",               types.gl_int},
        {"gl_MaxTessControlAtomicCounters",              types.gl_int},
        {"gl_MaxTessControlAtomicCounterBuffers",        types.gl_int},
        {"gl_MaxTessEvaluationInputComponents",          types.gl_int},
        {"gl_MaxTessEvaluationOutputComponents",         types.gl_int},
        {"gl_MaxTessEvaluationTextureImageUnits",        types.gl_int},
        {"gl_MaxTessEvaluationUniformComponents",        types.gl_int},
        {"gl_MaxTessEvaluationImageUniforms",            types.gl_int},
        {"gl_MaxTessEvaluationAtomicCounters",           types.gl_int},
        {"gl_MaxTessEvaluationAtomicCounterBuffers",     types.gl_int},
        {"gl_MaxGeometryInputComponents",                types.gl_int},
        {"gl_MaxGeometryOutputComponents",               types.gl_int},
        {"gl_MaxGeometryImageUniforms",                  types.gl_int},
        {"gl_MaxGeometryTextureImageUnits",              types.gl_int},
        {"gl_MaxGeometryOutputVertices",                 types.gl_int},
        {"gl_MaxGeometryTotalOutputComponents",          types.gl_int},
        {"gl_MaxGeometryUniformComponents",              types.gl_int},
        {"gl_MaxGeometryVaryingComponents",              types.gl_int},
        {"gl_MaxGeometryAtomicCounters",                 types.gl_int},
        {"gl_MaxGeometryAtomicCounterBuffers",           types.gl_int},
        {"gl_MaxFragmentImageUniforms",                  types.gl_int},
        {"gl_MaxFragmentInputComponents",                types.gl_int},
        {"gl_MaxFragmentUniformVectors",                 types.gl_int},
        {"gl_MaxFragmentUniformComponents",              types.gl_int},
        {"gl_MaxFragmentAtomicCounters",                 types.gl_int},
        {"gl_MaxFragmentAtomicCounterBuffers",           types.gl_int},
        {"gl_MaxDrawBuffers",                            types.gl_int},
        {"gl_MaxTextureImageUnits",                      types.gl_int},
        {"gl_MinProgramTexelOffset",                     types.gl_int},
        {"gl_MaxProgramTexelOffset",                     types.gl_int},
        {"gl_MaxImageUnits",                             types.gl_int},
        {"gl_MaxSamples",                                types.gl_int},
        {"gl_MaxImageSamples",                           types.gl_int},
        {"gl_MaxClipDistances",                          types.gl_int},
        {"gl_MaxCullDistances",                          types.gl_int},
        {"gl_MaxViewports",                              types.gl_int},
        {"gl_MaxComputeImageUniforms",                   types.gl_int},
        {"gl_MaxComputeWorkGroupCount",                  types.gl_ivec3},
        {"gl_MaxComputeWorkGroupSize",                   types.gl_ivec3},
        {"gl_MaxComputeUniformComponents",               types.gl_int},
        {"gl_MaxComputeTextureImageUnits",               types.gl_int},
        {"gl_MaxComputeAtomicCounters",                  types.gl_int},
        {"gl_MaxComputeAtomicCounterBuffers",            types.gl_int},
        {"gl_MaxCombinedTextureImageUnits",              types.gl_int},
        {"gl_MaxCombinedImageUniforms",                  types.gl_int},
        {"gl_MaxCombinedImageUnitsAndFragmentOutputs",   types.gl_int},
        {"gl_MaxCombinedShaderOutputResources",          types.gl_int},
        {"gl_MaxCombinedAtomicCounters",                 types.gl_int},
        {"gl_MaxCombinedAtomicCounterBuffers",           types.gl_int},
        {"gl_MaxCombinedClipAndCullDistances",           types.gl_int},
        {"gl_MaxAtomicCounterBindings",                  types.gl_int},
        {"gl_MaxAtomicCounterBufferSize",                types.gl_int},
        {"gl_MaxTransformFeedbackBuffers",               types.gl_int},
        {"gl_MaxTransformFeedbackInterleavedComponents", types.gl_int},
        // clang-format on
    };

    // sort alphabetically
    std::ranges::sort(
        globals, [](const BuiltinGlobal& a, const BuiltinGlobal& b) { return a.name < b.name; });

    return globals;
}

TargetInfo::FnList GLSLTargetInfo::create_fns(GLSLTypes& types) {
    FnList fns{};
    fns.reserve(builtins::builtin_fns.size());

    for (const auto& fn : builtins::builtin_fns) {
        OverloadList overloads{};
        overloads.reserve(fn.overloads.size());

        for (const auto& overload : fn.overloads) {
            TypeList arg_types{};
            arg_types.reserve(overload.arg_count);

            for (size_t i = 0; i < overload.arg_count; i++)
                arg_types.emplace_back(detail::builtin_ty_to_id(overload.args[i], types));

            overloads.emplace_back(std::move(arg_types),
                                   detail::builtin_ty_to_id(overload.ret_ty, types));
        }

        fns.emplace_back(fn.name, std::move(overloads));
    }

    return fns;
}

// see "5.9. Expressions" in the GLSL specs
TypeId GLSLTargetInfo::resolve_unop(TargetOpKind op, const TypeList& arg_types) const {
    assert(arg_types.size() == 1);
    assert(TargetOpKind::FirstUnop <= op && op <= TargetOpKind::LastUnop);

    TypeId ty = arg_types[0];
    if (ty.is_null())
        return TypeId::null_id();

    TypeId el_ty = gl_types.type_pool.el_type_of(ty, true);

    switch (op) {
        case TargetOpKind::UnopPlus:
        case TargetOpKind::UnopNeg:
        case TargetOpKind::UnopPreInc:
        case TargetOpKind::UnopPreDec:
        case TargetOpKind::UnopPostInc:
        case TargetOpKind::UnopPostDec:
            // " The arithmetic unary operators plus (*+*), negate (*-*), post- and pre-increment
            // and
            // decrement (*--* and *++*) operate on integer or floating-point values (including
            // vectors and matrices). All unary operators work component-wise on their operands.
            // These result with the same type they operated on"
            if (el_ty == gl_types.gl_float || el_ty == gl_types.gl_double ||
                el_ty == gl_types.gl_int || el_ty == gl_types.gl_uint)
                return ty;
            break;

        case TargetOpKind::UnopLogNot:
            // "The logical unary operator not (!). It operates only on a Boolean expression and
            // results in a Boolean expression."
            if (ty == gl_types.gl_bool)
                return ty;
            break;

        case TargetOpKind::UnopBitNeg:
            // "The one’s complement operator (~). The operand must be of type signed or unsigned
            // integer or integer vector, and the result is the one’s complement of its operand;
            // each bit of each component is complemented, including any sign bits."
            if (el_ty == gl_types.gl_int || el_ty == gl_types.gl_uint)
                return ty;
            break;

        default:
            // treats potential new generic target ops later as unsupported automatically
            break;
    }

    return TypeId::null_id();
}

// see "5.9. Expressions" and "4.1.10. Implicit Conversions" in the GLSL specs
TypeId GLSLTargetInfo::resolve_binop(TargetOpKind op, const TypeList& arg_types) const {
    assert(arg_types.size() == 2);
    assert(TargetOpKind::FirstBinop <= op && op <= TargetOpKind::LastBinop);

    if (arg_types[0].is_null() || arg_types[1].is_null())
        return TypeId::null_id();

    auto promote = [&](TypeId a, TypeId b) { // try promote scalar types
        assert(gl_types.is_gl_scalar_type(a) && gl_types.is_gl_scalar_type(b));

        if (a == b)
            return a;

        bool a2b = can_implicit_cast(a, b);
        bool b2a = can_implicit_cast(b, a);

        if (!a2b && !b2a)
            return TypeId::null_id();

        if (a2b != b2a)
            return a2b ? b : a;

        // ambigous conversion resolution (according to specs)
        assert(a2b && b2a);

        // this should NOT happen with GLSL implicit casting rules, as they form a DAG
        // so this is placed here to double check my math in debug builds
        // in release builds, the specs state how that should be handled anyways (though, again,
        // with A -> B or B -> A conversion, it shouldnt happen)
        assert(false && "see comment above");

        // "a floating-point type is chosen if either operand has a floating-point type"
        if (a == gl_types.gl_float || b == gl_types.gl_float)
            return gl_types.gl_float;
        if (a == gl_types.gl_double || b == gl_types.gl_double)
            return gl_types.gl_double;

        // "otherwise, an unsigned integer type is chosen if either operand has an unsigned integer
        // type"
        if (a == gl_types.gl_uint || b == gl_types.gl_uint)
            return gl_types.gl_uint;

        // "otherwise, a signed integer type is chosen"
        assert(a == gl_types.gl_int || b == gl_types.gl_int);
        return gl_types.gl_int;
    };

    TypeId lhs = arg_types[0];
    TypeId rhs = arg_types[1];

    const auto* lhs_td = &gl_types.type_pool.get_td(lhs);
    const auto* rhs_td = &gl_types.type_pool.get_td(rhs);

    // this shouldn't really happen, as theyre pointers to the returned ref-ed memory
    assert(lhs_td != nullptr && rhs_td != nullptr);

    bool is_lhs_scalar = gl_types.is_gl_scalar_type(lhs);
    bool is_rhs_scalar = gl_types.is_gl_scalar_type(rhs);
    bool is_lhs_vec    = gl_types.is_gl_vec_type(*lhs_td);
    bool is_rhs_vec    = gl_types.is_gl_vec_type(*rhs_td);
    bool is_lhs_mat    = gl_types.is_gl_mat_type(*lhs_td);
    bool is_rhs_mat    = gl_types.is_gl_mat_type(*rhs_td);
    bool is_lhs_arr    = lhs_td->is_array();
    bool is_rhs_arr    = rhs_td->is_array();
    bool is_lhs_struct = lhs_td->is_struct();
    bool is_rhs_struct = rhs_td->is_struct();

    if ((!is_lhs_scalar && !is_lhs_vec && !is_lhs_mat && !is_lhs_arr && !is_lhs_struct) ||
        (!is_rhs_scalar && !is_rhs_vec && !is_rhs_mat && !is_rhs_arr && !is_rhs_struct))
        return TypeId::null_id();

    if (is_lhs_struct != is_rhs_struct)
        return TypeId::null_id();

    // null id for structs (this also skips promotion later)
    TypeId lhs_el = is_lhs_scalar ? lhs : gl_types.type_pool.el_type_of(*lhs_td);
    TypeId rhs_el = is_rhs_scalar ? rhs : gl_types.type_pool.el_type_of(*rhs_td);

    auto lhs_mi = MatrixTD::MatrixInfo{0, 0, TypeId::null_id()};
    auto rhs_mi = MatrixTD::MatrixInfo{0, 0, TypeId::null_id()};

    if (is_lhs_mat)
        lhs_mi = lhs_td->as<MatrixTD>().get_info(gl_types.type_pool);

    if (is_rhs_mat)
        rhs_mi = rhs_td->as<MatrixTD>().get_info(gl_types.type_pool);

    // Logical and shift ops dont need promotion

    // "The logical binary operators and (&&), or (||), and exclusive or (^^) operate only on two
    // Boolean expressions and result in a Boolean expression."
    if (op == TargetOpKind::BinopLogAnd || op == TargetOpKind::BinopLogOr ||
        op == TargetOpKind::BinopLogXor) {
        if (lhs != gl_types.gl_bool || rhs != gl_types.gl_bool)
            return TypeId::null_id();

        return gl_types.gl_bool;
    }

    // "The shift operators (<<) and (>>). For both operators, the operands must be signed or
    // unsigned integers or integer vectors. One operand can be signed while the other is unsigned.
    // In all cases, the resulting type will be the same type as the left operand. If the first
    // operand is a scalar, the second operand has to be a scalar as well. If the first operand is a
    // vector, the second operand must be a scalar or a vector with the same size as the first
    // operand, and the result is computed component-wise."
    if (op == TargetOpKind::BinopLShift || op == TargetOpKind::BinopRShift) {
        if (!(is_lhs_scalar || is_lhs_vec) || !(is_rhs_scalar || is_rhs_vec))
            return TypeId::null_id();

        if ((lhs_el != gl_types.gl_int && lhs_el != gl_types.gl_uint) ||
            (rhs_el != gl_types.gl_int && rhs_el != gl_types.gl_uint)) {
            return TypeId::null_id();
        }

        if (is_lhs_scalar && !is_rhs_scalar)
            return TypeId::null_id();

        if (is_lhs_vec && is_rhs_vec &&
            lhs_td->as<VectorTD>().component_count != rhs_td->as<VectorTD>().component_count)
            return TypeId::null_id();

        return lhs;
    }

    TypeId prom_el = lhs_el;

    if (lhs_el != rhs_el) {
        // arrays cannot be element-wise implicit converted
        if (is_lhs_arr || is_rhs_arr)
            return TypeId::null_id();

        prom_el = promote(lhs_el, rhs_el);
        if (prom_el.is_null())
            return TypeId::null_id();

        lhs_el = prom_el;
        rhs_el = prom_el;
    }

    TypeId orig_lhs = lhs;
    TypeId orig_rhs = rhs;

    if (is_lhs_scalar)
        lhs = prom_el;

    if (is_rhs_scalar)
        rhs = prom_el;

    if (is_lhs_vec)
        lhs = gl_types.gl_TvecN(prom_el, lhs_td->as<VectorTD>().component_count);

    if (is_rhs_vec)
        rhs = gl_types.gl_TvecN(prom_el, rhs_td->as<VectorTD>().component_count);

    if (is_lhs_mat || is_rhs_mat) {
        if (prom_el != gl_types.gl_float && prom_el != gl_types.gl_double)
            return TypeId::null_id();

        if (is_lhs_mat) {
            lhs                   = gl_types.gl_TmatNxM(prom_el, lhs_mi.cols, lhs_mi.rows);
            lhs_mi.component_type = prom_el;
        }

        if (is_rhs_mat) {
            rhs                   = gl_types.gl_TmatNxM(prom_el, rhs_mi.cols, rhs_mi.rows);
            rhs_mi.component_type = prom_el;
        }
    }

    if (lhs != orig_lhs)
        lhs_td = &gl_types.type_pool.get_td(lhs);
    if (rhs != orig_rhs)
        rhs_td = &gl_types.type_pool.get_td(rhs);

    switch (op) {
        case TargetOpKind::BinopAdd:
        case TargetOpKind::BinopSub:
        case TargetOpKind::BinopMul:
        case TargetOpKind::BinopDiv: {
            // "The two operands are scalars. In this case the operation is applied, resulting in a
            // scalar."
            if (is_lhs_scalar && is_rhs_scalar)
                return prom_el;

            // "One operand is a scalar, and the other is a vector or matrix. In this case, the
            // scalar operation is applied independently to each component of the vector or matrix,
            // resulting in the same size vector or matrix."
            if ((is_lhs_vec || is_lhs_mat) && is_rhs_scalar)
                return lhs;
            if ((is_rhs_vec || is_rhs_mat) && is_lhs_scalar)
                return rhs;

            // "The two operands are vectors of the same size. In this case, the operation is done
            // component-wise resulting in the same size vector"
            if (is_lhs_vec && is_rhs_vec &&
                lhs_td->as<VectorTD>().component_count == rhs_td->as<VectorTD>().component_count)
                return rhs;

            // "The operator is add (+), subtract (-), or divide (/), and the operands are matrices
            // with the same number of rows and the same number of columns. In this case, the
            // operation is done component-wise resulting in the same size matrix."
            bool both_mat = is_lhs_mat && is_rhs_mat;
            if (both_mat && op != TargetOpKind::BinopMul) {
                assert(!lhs_mi.component_type.is_null() && !rhs_mi.component_type.is_null());

                if (lhs_mi == rhs_mi)
                    return lhs;
            }

            // "The operator is multiply (*), where both operands are matrices or one operand is a
            // vector and the other a matrix. A right vector operand is treated as a column vector
            // and a left vector operand as a row vector. In all these cases, it is required that
            // the number of columns of the left operand is equal to the number of rows of the right
            // operand. Then, the multiply (*) operation does a linear algebraic multiply, yielding
            // an object that has the same number of rows as the left operand and the same number of
            // columns as the right operand."
            bool one_mat_one_vec = (is_lhs_mat && is_rhs_vec) || (is_rhs_mat && is_lhs_vec);
            if ((both_mat || one_mat_one_vec) && op == TargetOpKind::BinopMul) {
                // left vector operand is treated as a row vector
                uint32_t lhs_cols = is_lhs_mat ? lhs_mi.cols : 1;
                uint32_t lhs_rows =
                    is_lhs_mat ? lhs_mi.rows : lhs_td->as<VectorTD>().component_count;
                // right vector operand is treated as a column vector
                uint32_t rhs_cols =
                    is_rhs_mat ? rhs_mi.cols : rhs_td->as<VectorTD>().component_count;
                uint32_t rhs_rows = is_rhs_mat ? rhs_mi.rows : 1;

                if (lhs_cols == rhs_rows) {
                    if (lhs_rows == 1 && rhs_cols == 1)
                        return prom_el;

                    if (lhs_rows == 1)
                        return gl_types.gl_TvecN(prom_el, rhs_cols);

                    if (rhs_cols == 1)
                        return gl_types.gl_TvecN(prom_el, lhs_rows);

                    if (prom_el != gl_types.gl_float && prom_el != gl_types.gl_double)
                        return TypeId::null_id();

                    return gl_types.gl_TmatNxM(prom_el, rhs_cols, lhs_rows);
                }
            }

            // "All other cases result in a compile-time error."
            return TypeId::null_id();
        }

        case TargetOpKind::BinopMod: {
            // "The operator modulus (%) operates on signed or unsigned integers or integer vectors.
            // If the fundamental types in the operands do not match, then the conversions from
            // “Implicit Conversions” are applied to create matching types. The operands cannot be
            // vectors of differing size; this is a compile-time error. If one operand is a scalar
            // and the other vector, then the scalar is applied component-wise to the vector,
            // resulting in the same type as the vector. If both are vectors of the same size, the
            // result is computed component-wise. The resulting value is undefined for any component
            // computed with a second operand that is zero, while results for other components with
            // non-zero second operands remain defined. If both operands are non negative, then the
            // remainder is non-negative. Results are undefined if one or both operands are
            // negative. The operator modulus (%) is not defined for any other data types
            // (non-integer types). "
            if (prom_el != gl_types.gl_int && prom_el != gl_types.gl_uint)
                return TypeId::null_id();

            if (is_lhs_scalar && is_rhs_scalar)
                return prom_el;

            if (is_lhs_vec || is_rhs_vec) {
                if (is_lhs_vec && is_rhs_vec) {
                    return lhs_td->as<VectorTD>().component_count ==
                                   rhs_td->as<VectorTD>().component_count
                               ? lhs
                               : TypeId::null_id();
                }

                if (is_lhs_scalar || is_rhs_scalar)
                    return is_lhs_vec ? lhs : rhs;

                return TypeId::null_id();
            }

            return TypeId::null_id();
        }

        case TargetOpKind::BinopLt:
        case TargetOpKind::BinopLeq:
        case TargetOpKind::BinopGt:
        case TargetOpKind::BinopGeq: {
            // "The relational operators greater than (>), less than (<), greater than or equal
            // (>=), and less than or equal (<=) operate only on scalar integer and scalar
            // floating-point expressions. The result is scalar Boolean. Either the operands' types
            // must match, or the conversions from section “Implicit Conversions” will be applied to
            // obtain matching types"
            if (!is_lhs_scalar || !is_rhs_scalar ||
                (prom_el != gl_types.gl_int && prom_el != gl_types.gl_uint &&
                 prom_el != gl_types.gl_float && prom_el != gl_types.gl_double))
                return TypeId::null_id();

            return gl_types.gl_bool;
        }

        case TargetOpKind::BinopEq:
        case TargetOpKind::BinopNeq: {
            // "The equality operators equal (==), and not equal (!=) operate on all types except
            // opaque types, aggregates that contain opaque types, subroutine uniforms, and
            // aggregates that contain subroutine uniforms. They result in a scalar Boolean. If the
            // operand types do not match, then there must be a conversion from “Implicit
            // Conversions” applied to one operand that can make them match, in which case this
            // conversion is done. For vectors, matrices, structures, and arrays, all components,
            // members, or elements of one operand must equal the corresponding components, members,
            // or elements in the other operand for the operands to be considered equal."
            assert(!is_lhs_struct || is_rhs_struct);

            if (lhs != rhs)
                return TypeId::null_id();

            // if promotion failed, we've already returned null
            return gl_types.gl_bool;
        }

        case TargetOpKind::BinopBitAnd:
        case TargetOpKind::BinopBitOr:
        case TargetOpKind::BinopBitXor: {
            // "The bitwise operators and (&), exclusive-or (^), and inclusive-or (|). The operands
            // must be of type signed or unsigned integers or integer vectors. The operands cannot
            // be vectors of differing size; this is a compile-time error. If one operand is a
            // scalar and the other a vector, the scalar is applied component-wise to the vector,
            // resulting in the same type as the vector. If the fundamental types in the operands do
            // not match, then the conversions from “Implicit Conversions” are applied to create
            // matching types, and this will be the resulting fundamental type."
            if (prom_el != gl_types.gl_int && prom_el != gl_types.gl_uint)
                return TypeId::null_id();

            if (!(is_lhs_scalar || is_lhs_vec) || !(is_rhs_scalar || is_rhs_vec))
                return TypeId::null_id();

            if (is_lhs_vec && is_rhs_vec &&
                lhs_td->as<VectorTD>().component_count != rhs_td->as<VectorTD>().component_count)
                return TypeId::null_id();

            if (is_lhs_scalar && is_rhs_scalar)
                return prom_el;

            if (is_lhs_vec != is_rhs_vec)
                return is_lhs_vec ? lhs : rhs;

            return lhs;
        }

        case TargetOpKind::BinopLogAnd:
        case TargetOpKind::BinopLogOr:
        case TargetOpKind::BinopLogXor:
        case TargetOpKind::BinopLShift:
        case TargetOpKind::BinopRShift: {
            assert(false && "binop should have been handled pre-promotion");
            break;
        }

        default:
            // potential new target ops are automatically treated as unsupported
            break;
    }

    return TypeId::null_id();
}

TypeId GLSLTargetInfo::builtin_op_ret_ty(TargetOpKind op, const TypeList& arg_types) const {
    if (arg_types.size() == 1 && TargetOpKind::FirstUnop <= op && op <= TargetOpKind::LastUnop)
        return resolve_unop(op, arg_types);

    if (arg_types.size() == 2 && TargetOpKind::FirstBinop <= op && op <= TargetOpKind::LastBinop)
        return resolve_binop(op, arg_types);

    return TypeId::null_id();
}

bool GLSLTargetInfo::valid_ctor_call(TypeId target, const TypeList& arg_types) const {
    if (target.is_null())
        return false;

    if (!gl_types.is_gl_type(target) || target == gl_types.gl_void || arg_types.empty())
        return false;

    if (arg_types.size() == 1 && gl_types.is_gl_scalar_type(arg_types[0]))
        return true;

    // matrix from matrix is always allowed
    bool is_target_mat = gl_types.is_gl_mat_type(target);
    if (is_target_mat && gl_types.is_gl_mat_type(arg_types[0]))
        return arg_types.size() == 1;

    auto get_comps = [this](TypeId ty) -> uint8_t {
        assert(!ty.is_null());
        assert(gl_types.is_gl_type(ty));
        assert(ty != gl_types.gl_void);

        const auto& td = gl_types.type_pool.get_td(ty);

        if (td.is_scalar())
            return 1;

        if (td.is_vector()) {
            return static_cast<uint8_t>(td.as<VectorTD>().component_count);
        }

        if (td.is_matrix()) {
            MatrixTD mat_td = td.as<MatrixTD>();
            assert(!mat_td.column_type_id.is_null());

            const auto& col_td = gl_types.type_pool.get_td(mat_td.column_type_id);
            assert(col_td.is_vector());

            return static_cast<uint8_t>(mat_td.column_count *
                                        col_td.as<VectorTD>().component_count);
        }

        return 0;
    };

    uint8_t req_comps = get_comps(target);
    assert(req_comps > 0);

    uint8_t actual_comps = 0;
    for (TypeId arg : arg_types) {
        // overspecifying
        if (actual_comps >= req_comps)
            return false;

        if (!gl_types.is_gl_type(arg) || arg == gl_types.gl_void)
            return false;

        // "If a matrix argument is given to a matrix constructor, it is a compile-time error to
        // have any other arguments."
        if (is_target_mat && gl_types.is_gl_mat_type(arg))
            return false;

        uint8_t arg_comps = get_comps(arg);
        if (arg_comps == 0)
            return false;

        actual_comps += arg_comps;
    }

    return actual_comps >= req_comps;
}

// implemented according to the table in the 4.1.10 (implicit conversions) section of the specs
bool GLSLTargetInfo::can_implicit_cast(TypeId src_ty, TypeId dest_ty) const {
    const auto& glt = gl_types;

    const auto can_cast_scalar = [&glt](TypeId src, TypeId dest) {
        assert(glt.is_gl_scalar_type(dest));

        if (dest == glt.gl_uint)
            return src == glt.gl_int;

        if (dest == glt.gl_float)
            return src == glt.gl_int || src == glt.gl_uint;

        if (dest == glt.gl_double)
            return src == glt.gl_int || src == glt.gl_uint || src == glt.gl_float;

        return false;
    };

    if (!glt.is_gl_type(src_ty) || !glt.is_gl_type(dest_ty))
        return false;

    if (src_ty == dest_ty)
        return true;

    // int -> uint
    // int, uint -> float
    // int, uint, float -> double
    if (glt.is_gl_scalar_type(dest_ty))
        return can_cast_scalar(src_ty, dest_ty);

    const auto& src_td  = glt.type_pool.get_td(src_ty);
    const auto& dest_td = glt.type_pool.get_td(dest_ty);

    // ivecn -> uvecn
    // ivecn, uvecn -> vecn
    // ivecn, uvecn, vecn -> dvecn
    if (src_td.is_vector() && dest_td.is_vector()) {
        VectorTD src_vec  = src_td.as<VectorTD>();
        VectorTD dest_vec = dest_td.as<VectorTD>();

        if (src_vec.component_count != dest_vec.component_count)
            return false;

        return can_cast_scalar(src_vec.component_type_id, dest_vec.component_type_id);
    }

    // matnxm -> dmatnxm
    if (src_td.is_matrix() && dest_td.is_matrix()) {
        MatrixTD src_mat  = src_td.as<MatrixTD>();
        MatrixTD dest_mat = dest_td.as<MatrixTD>();

        if (src_mat.column_count != dest_mat.column_count)
            return false;

        VectorTD src_col  = glt.type_pool.get_td(src_mat.column_type_id).as<VectorTD>();
        VectorTD dest_col = glt.type_pool.get_td(dest_mat.column_type_id).as<VectorTD>();

        if (src_col.component_count != dest_col.component_count)
            return false;

        return src_col.component_type_id == glt.gl_float &&
               dest_col.component_type_id == glt.gl_double;
    }

    return false;
}

} // namespace stc::glsl
