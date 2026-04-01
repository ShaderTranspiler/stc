#include "frontend/jl/parser.h"
#include <algorithm>
#include <bit>
#include <utility>

// struct layouts were taken from:
// https://github.com/JuliaLang/julia/blob/master/base/boot.jl

namespace {

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

STC_FORCE_INLINE jl_expr_t* is_expr(jl_value_t* value, jl_sym_t* head) {
    if (value == nullptr || !jl_is_expr(value))
        return nullptr;

    auto* expr = reinterpret_cast<jl_expr_t*>(value);
    return expr->head == head ? expr : nullptr;
}

class jl_parse_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

} // namespace

namespace stc::jl {

// TODO
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
TypeId JLParser::resolve_type([[maybe_unused]] jl_value_t* type) {
    return TypeId::null_id();
}

NodeId JLParser::parse(jl_value_t* node) {
    if (jl_is_linenumbernode(node)) {
        // ! line::Int
        intptr_t line = jl_unbox_long(safe_fieldref(node, 0, "line"));

        // ! file::Union{Symbol, Nothing}
        jl_value_t* file_val = safe_fieldref(node, 1, "file");

        if (jl_is_symbol(file_val)) {
            auto* file_sym = reinterpret_cast<jl_sym_t*>(file_val);
            std::ignore    = ctx.src_info_pool.get_file(jl_symbol_name(file_sym));
        } else {
            std::ignore = ctx.src_info_pool.get_file("<unknown>");
        }

        cur_loc = ctx.src_info_pool.get_location(line, 1U);

        return NodeId::null_id();
    }

    // parsed symbols are treated as declaration references, with the symbol as the target
    // the actual decl it points to will be resolved by sema
    if (jl_is_symbol(node)) {
        auto* sym = reinterpret_cast<jl_sym_t*>(node);

        if (sym == sym_cache.nothing)
            return emplace_node<NothingLiteral>(cur_loc);

        SymbolId sym_id = ctx.sym_pool.get_id(jl_symbol_name(sym));
        NodeId sym_lit  = emplace_node<SymbolLiteral>(cur_loc, sym_id);

        return emplace_node<DeclRefExpr>(cur_loc, sym_lit);
    }

    // global refs are handled similarly to symbols, but with a module name symbol included
    if (jl_is_globalref(node)) {
        // ! mod::Module
        auto* module = safe_cast<jl_module_t>(safe_fieldref(node, 0, "mod"));
        // ! name::Symbol
        auto* name   = safe_cast<jl_sym_t>(safe_fieldref(node, 1, "name"));

        ModuleId mod_id  = ctx.module_pool.get_id(module);
        SymbolId name_id = ctx.sym_pool.get_id(jl_symbol_name(name));

        NodeId glob_ref = emplace_node<GlobalRef>(cur_loc, mod_id, name_id);

        return emplace_node<DeclRefExpr>(cur_loc, glob_ref);
    }

    // quote nodes are simply unwrapped and parsed
    if (jl_is_quotenode(node)) {
        // ! value::Any
        jl_value_t* inner_val = safe_fieldref(node, 0, "value");

        return parse(inner_val);
    }

    if (jl_is_expr(node))
        return parse_expr(safe_cast<jl_expr_t>(node));

    if (jl_is_bool(node)) {
        // jl_unbox_bool returns an uint8_t
        bool value = static_cast<bool>(jl_unbox_bool(node));

        return emplace_node<BoolLiteral>(cur_loc, value);
    }

#define HANDLE_LITERAL(jl_type, value_type, node_type)                                             \
    static_assert(std::constructible_from<node_type, SrcLocationId, value_type>);                  \
    if (jl_is_##jl_type(node)) {                                                                   \
        value_type value = jl_unbox_##jl_type(node);                                               \
        return emplace_node<node_type>(cur_loc, value);                                            \
    }

    HANDLE_LITERAL(int32, int32_t, Int32Literal)
    HANDLE_LITERAL(int64, int64_t, Int64Literal)
    HANDLE_LITERAL(uint8, uint8_t, UInt8Literal)
    HANDLE_LITERAL(uint16, uint16_t, UInt16Literal)
    HANDLE_LITERAL(uint32, uint32_t, UInt32Literal)
    HANDLE_LITERAL(uint64, uint64_t, UInt64Literal)

#undef HANDLE_LITERAL

    // TODO: TEST ON SOME BE VM!!
    if (jl_typeis(node, type_cache.uint128)) {
        const auto* data = reinterpret_cast<const uint64_t*>(node);
        uint64_t hi, lo; // NOLINT(cppcoreguidelines-init-variables)

        if constexpr (std::endian::native == std::endian::little) {
            lo = data[0];
            hi = data[1];
        } else if constexpr (std::endian::native == std::endian::big) {
            lo = data[1];
            hi = data[0];
        } else {
            throw std::runtime_error{
                "Using UInt128 literals is not supported on mixed-endian systems"};
        }

        return emplace_node<UInt128Literal>(cur_loc, hi, lo);
    }

    if (jl_typeis(node, jl_float32_type)) {
        float value = jl_unbox_float32(node);

        return emplace_node<Float32Literal>(cur_loc, value);
    }

    if (jl_typeis(node, jl_float64_type)) {
        double value = jl_unbox_float64(node);

        return emplace_node<Float64Literal>(cur_loc, value);
    }

    if (jl_is_string(node)) {
        const char* str_data = jl_string_ptr(node);
        size_t str_len       = jl_string_len(node);

        std::string str{str_data, str_len};

        return emplace_node<StringLiteral>(cur_loc, std::move(str));
    }

    // rest of the nodes should be raw Julia objects directly injected into the AST
    // sema can decide what to do with these later
    // for known datatypes, they can be captured with their current value, or an error can be thrown

    auto* datatype     = safe_cast<jl_datatype_t>(jl_typeof(node));
    auto* type_name    = jl_symbol_name(datatype->name->name);
    SymbolId tname_sid = ctx.sym_pool.get_id(type_name);

    return emplace_node<OpaqueNode>(cur_loc, tname_sid, node);
}

// parses the code argument using Julia's Meta.parse and invokes the regular parsing pipeline on it
NodeId JLParser::parse_code(std::string_view code) {
    jl_value_t* code_jl_str = nullptr;
    jl_value_t* parsed_expr = nullptr;
    JL_GC_PUSH2(&code_jl_str, &parsed_expr);

    jl_value_t* meta_mod_val = jl_get_global(jl_base_module, jl_symbol("Meta"));
    if (meta_mod_val == nullptr || !jl_is_module(meta_mod_val)) {
        JL_GC_POP();
        throw std::logic_error{"Failed to look up Meta module inside Base"};
    }
    jl_module_t* meta_mod = reinterpret_cast<jl_module_t*>(meta_mod_val);

    jl_function_t* parse_fn = jl_get_global(meta_mod, jl_symbol("parse"));
    if (parse_fn == nullptr) {
        JL_GC_POP();
        throw std::logic_error{"Failed to look up parse function inside the Meta module"};
    }

    // implemented as a simple memcpy in libjulia, avoids strlen (==> null termination agnostic)
    code_jl_str = jl_pchar_to_string(code.data(), code.size());

    parsed_expr = jl_call1(parse_fn, code_jl_str);

    jl_value_t* ex = jl_exception_occurred();
    if (ex != nullptr) {
        const char* ex_type_str = jl_typeof_str(ex);
        jl_static_show(jl_stderr_stream(), ex);
        jl_exception_clear();

        JL_GC_POP();

        throw std::runtime_error{std::format(
            "Julia exception while trying to parse code string using Meta.parse: {}", ex_type_str)};
    }

    NodeId parser_result = parse(parsed_expr);

    JL_GC_POP();

    return parser_result;
}

NodeId JLParser::parse_expr(jl_expr_t* expr) {
    jl_sym_t* head = expr->head;
    size_t nargs   = jl_expr_nargs(expr);

    if (head == sym_cache.block)
        return parse_block(expr, nargs);

    if (head == sym_cache.call)
        return parse_call(expr, nargs);

    if (head == sym_cache.if_)
        return parse_if(expr, nargs);

    if (head == sym_cache.while_)
        return parse_while(expr, nargs);

    if (head == sym_cache.eq)
        return parse_assignment(expr, nargs);

    if (head == sym_cache.global || head == sym_cache.local)
        return parse_var_decl(expr, nargs);

    if (head == sym_cache.break_)
        return emplace_node<BreakStmt>(cur_loc);

    if (head == sym_cache.continue_)
        return emplace_node<ContinueStmt>(cur_loc);

    // TODO: print cur_loc and julia dump node
    throw jl_parse_error{"Unrecognized Expr node encountered in Julia source code"};
}

NodeId JLParser::parse_var_decl(jl_expr_t* expr, size_t nargs) {
    assert(expr->head == sym_cache.global || expr->head == sym_cache.local);
    auto scope = expr->head == sym_cache.global ? ScopeType::Global : ScopeType::Local;

    SrcLocationId decl_loc = cur_loc;

    if (nargs != 1)
        throw jl_parse_error{"Variable declaration with more/less than one arg"};

    jl_value_t* inner = jl_exprarg(expr, 0);
    jl_value_t* id    = nullptr;
    jl_value_t* type  = nullptr;
    jl_value_t* init  = nullptr;

    if (auto* assignment_expr = is_expr(inner, sym_cache.eq)) {
        if (jl_expr_nargs(assignment_expr) != 2)
            throw jl_parse_error{"Assignment expression with more/less than two args"};

        inner = jl_exprarg(assignment_expr, 0);
        init  = jl_exprarg(assignment_expr, 1);
    }

    if (auto* typed_expr = is_expr(inner, sym_cache.double_col)) {
        if (jl_expr_nargs(typed_expr) != 2)
            throw jl_parse_error{"Type annotation expression with more/less than two args"};

        id   = jl_exprarg(typed_expr, 0);
        type = jl_exprarg(typed_expr, 1);
    }

    if (jl_is_symbol(inner)) {
        id = inner;
    }

    if (id == nullptr || !jl_is_symbol(id))
        throw jl_parse_error{
            "Invalid variable declaration expression, couldn't unwrap identifier symbol"};

    SymbolId id_sym = ctx.sym_pool.get_id(jl_symbol_name(safe_cast<jl_sym_t>(id)));

    return emplace_node<VarDecl>(decl_loc, id_sym,
                                 type != nullptr ? resolve_type(type) : TypeId::null_id(), scope,
                                 init != nullptr ? parse(init) : NodeId::null_id());
}

NodeId JLParser::parse_assignment(jl_expr_t* expr, size_t nargs) {
    assert(expr->head == sym_cache.eq);

    if (nargs != 2)
        throw jl_parse_error{"Assignment expression with more/less than two args"};

    SrcLocationId outer_loc = cur_loc;

    NodeId parsed_lhs = parse(jl_exprarg(expr, 0));
    NodeId parsed_rhs = parse(jl_exprarg(expr, 1));

    return emplace_node<Assignment>(outer_loc, parsed_lhs, parsed_rhs);
}

NodeId JLParser::parse_block(jl_expr_t* expr, size_t nargs) {
    assert(expr->head == sym_cache.block);

    SrcLocationId cmpd_loc = cur_loc;
    std::vector<NodeId> inner_exprs{};
    inner_exprs.reserve(nargs);

    for (size_t i = 0; i < nargs; i++) {
        NodeId parsed_arg = parse(jl_exprarg(expr, i));

        if (parsed_arg.is_null())
            continue;

        inner_exprs.push_back(parsed_arg);

        // if block is not empty, make its location point to the first inner line, rather than the
        // last outer one
        if (inner_exprs.size() == 1) {
            cmpd_loc = cur_loc;
        }
    }

    return emplace_node<CompoundExpr>(cmpd_loc, std::move(inner_exprs));
}

NodeId JLParser::parse_call(jl_expr_t* expr, size_t nargs) {
    assert(expr->head == sym_cache.call);

    SrcLocationId call_loc = cur_loc;

    if (nargs == 0)
        throw jl_parse_error{"Function call expression with zero arguments"};

    NodeId parsed_target_fn = parse(jl_exprarg(expr, 0));

    std::vector<NodeId> params;
    for (size_t i = 1; i < nargs; i++) {
        jl_value_t* arg = jl_exprarg(expr, i);

        // handle kwargs
        auto* arg_expr = try_cast<jl_expr_t>(arg);
        if (arg_expr != nullptr && arg_expr->head == sym_cache.parameters) {
            // TODO
            // args traversal where arg is either Expr(:kw, k, v) or Symbol (<=> Expr(:kw, k, k))
            throw std::runtime_error{"Keyword arguments are not supported yet"};
        }

        NodeId parsed_arg = parse(arg);
        params.push_back(parsed_arg);
    }

    return emplace_node<FunctionCall>(call_loc, parsed_target_fn, std::move(params));
}

NodeId JLParser::parse_if(jl_expr_t* expr, size_t nargs) {
    assert(expr->head == sym_cache.if_ || expr->head == sym_cache.elseif);

    SrcLocationId if_loc = cur_loc;

    if (nargs < 2)
        throw jl_parse_error{
            "If expr without at least two args (assumed: condition + true branch)"};

    NodeId parsed_cond  = parse(jl_exprarg(expr, 0));
    NodeId parsed_true  = parse(jl_exprarg(expr, 1));
    NodeId parsed_false = NodeId::null_id();

    if (nargs == 3) {
        jl_value_t* false_branch = jl_exprarg(expr, 2);
        auto* false_branch_expr  = try_cast<jl_expr_t>(false_branch);

        if (false_branch_expr != nullptr && false_branch_expr->head == sym_cache.elseif) {
            parsed_false = parse_if(false_branch_expr, jl_expr_nargs(false_branch_expr));
        } else {
            parsed_false = parse(false_branch);
        }
    }

    return emplace_node<IfExpr>(if_loc, parsed_cond, parsed_true, parsed_false);
}

NodeId JLParser::parse_while(jl_expr_t* expr, size_t nargs) {
    assert(expr->head == sym_cache.while_);

    if (nargs != 2)
        throw jl_parse_error{"Unexpected while expr arg count"};

    SrcLocationId while_loc = cur_loc;

    NodeId parsed_cond = parse(jl_exprarg(expr, 0));
    NodeId parsed_body = parse(jl_exprarg(expr, 1));

    return emplace_node<WhileExpr>(while_loc, parsed_cond, parsed_body);
}

NodeId JLParser::parse_return(jl_expr_t* expr, size_t nargs) {
    assert(expr->head == sym_cache.return_);

    if (nargs == 0)
        return emplace_node<ReturnStmt>(cur_loc);

    if (nargs != 1)
        throw jl_parse_error{"Return expr with more than one arg (assumed 0 or 1)"};

    SrcLocationId ret_loc = cur_loc;
    NodeId parsed_inner   = parse(jl_exprarg(expr, 0));

    return emplace_node<ReturnStmt>(ret_loc, parsed_inner);
}

} // namespace stc::jl
