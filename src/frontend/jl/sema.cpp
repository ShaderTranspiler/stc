#include "base.h"

#include "frontend/jl/dumper.h"
#include "frontend/jl/sema.h"

namespace {

using namespace stc::jl;

[[nodiscard]] STC_FORCE_INLINE std::string_view scope_str(ScopeType scope) {
    switch (scope) {
        case ScopeType::Global:
            return "global";

        case ScopeType::Local:
            return "local";
    }

    throw std::logic_error{"Unaccounted ScopeType value in scope_str"};
}

[[nodiscard]] STC_FORCE_INLINE ScopeType bt_to_st(BindingType bt) {
    if (bt == BindingType::Captured)
        throw std::logic_error{"Trying to convert BindingType with value Captured to a ScopeType"};

    assert((bt == BindingType::Global || bt == BindingType::Local) && "unaccounted binding type");
    return bt == BindingType::Global ? ScopeType::Global : ScopeType::Local;
}

} // namespace

namespace stc::jl {

void JLSema::dump(const Expr& expr) const {
    JLDumper dumper{ctx, std::cerr};
    dumper.visit(ctx.calculate_node_id(&expr, expr.kind()));
}

TypeId JLSema::fail(std::string_view msg, const Expr& expr) {
    std::cerr << '\n';

    auto [loc, file] = ctx.src_info_pool.get_loc_and_file(expr.location);
    error(file, loc, msg);
    _success = false;

    std::cerr << "The above error was emitted while processing the following node:\n";
    dump(expr);

    return TypeId::null_id();
}

TypeId JLSema::warn(std::string_view msg, const Expr& expr) {
    std::cerr << '\n';

    auto [loc, file] = ctx.src_info_pool.get_loc_and_file(expr.location);
    warning(file, loc, msg);

    std::cerr << "The above warning was emitted while processing the following node:\n";
    dump(expr);

    return TypeId::null_id();
}

TypeId JLSema::internal_error(std::string_view msg, const Expr& expr) {
    std::cerr << '\n';

    auto [loc, file] = ctx.src_info_pool.get_loc_and_file(expr.location);
    stc::internal_error(file, loc, msg);
    _success = false;

    std::cerr << "The above error occured while processing the following node:\n";
    dump(expr);

    return TypeId::null_id();
}

std::string JLSema::type_str(TypeId id) const {
    return to_string(id, ctx.type_pool, ctx.sym_pool);
}

TypeId JLSema::visit_default_case() {
    _success = false;
    stc::internal_error("unexpected null id node found in the AST");
    return TypeId::null_id();
}

void JLSema::check(Expr& expr, TypeId expected_type) {
    auto prev_expected  = this->expected_type;
    this->expected_type = expected_type;

    TypeId actual_type = this->visit(&expr);

    // TODO: subsumption

    if (actual_type != expected_type) {
        fail(std::format("type mismatch during type checking: expected {}, got {}",
                         type_str(expected_type), type_str(actual_type)),
             expr);

        return;
    }

    expr.type = actual_type;

    this->expected_type = prev_expected;
}

TypeId JLSema::infer(Expr& expr) {
    auto prev_expected = expected_type;
    expected_type      = TypeId::null_id();

    TypeId inferred = visit(&expr);

    if (inferred.is_null())
        return fail("couldn't infer type for node during type checking", expr);

    expr.type = inferred;

    expected_type = prev_expected;

    return inferred;
}

TypeId JLSema::visit_VarDecl(VarDecl& vdecl) {
    if (vdecl.identifier.is_null())
        return fail("variable declaration with null as identifier", vdecl);

    TypeId result_type = TypeId::null_id();

    bool has_type = !vdecl.type.is_null();
    bool has_init = !vdecl.initializer.is_null();

    auto expected_bt = binding_of(vdecl.identifier);

    if (!expected_bt.has_value())
        return internal_error(std::format("symbol resolution pass failed to infer binding type for "
                                          "a variable declaration's symbol: '{}'",
                                          ctx.get_sym(vdecl.identifier)),
                              vdecl);
    else if (*expected_bt == BindingType::Captured)
        return internal_error("wrongfully inferred binding type of Captured for a symbol that is "
                              "explicitly declared in scope",
                              vdecl);

    ScopeType expected_st = bt_to_st(*expected_bt);

    if (vdecl.scope() != expected_st)
        return internal_error(
            std::format(
                "scope mismatch for target scope of variable declaration. inferred {}, found {}",
                scope_str(expected_st), scope_str(vdecl.scope())),
            vdecl);

    // TODO: lazy infer declared var type
    if (!has_type && !has_init)
        return fail("variable declaration without neither a type annotation, or an initializer is "
                    "currently not allowed",
                    vdecl);

    if (has_type) {
        if (has_init)
            check(vdecl.initializer, vdecl.type);

        result_type = vdecl.type;
    } else {
        result_type = infer(vdecl.initializer);
    }

    NodeId decl_id = ctx.calculate_node_id(&vdecl, vdecl.kind());
    st_register(vdecl.identifier, decl_id);

    return result_type;
}

TypeId JLSema::visit_ParamDecl(ParamDecl& pdecl) {
    // TODO: support for kwargs (here and in MethodDecl visitor)
    if (pdecl.is_kwarg())
        return fail("kwargs are not supported yet", pdecl);

    // TODO: generalize not found / not as expected errors into lookup helpers
    auto expected_bt = binding_of(pdecl.identifier);

    if (!expected_bt.has_value())
        return internal_error(
            std::format(
                "symbol resolution pass failed to infer binding type for parameter symbol '{}'",
                ctx.get_sym(pdecl.identifier)),
            pdecl);

    if (*expected_bt != BindingType::Local)
        return internal_error(
            std::format("wrongfully infered non-local binding type for parameter symbol '{}'",
                        ctx.get_sym(pdecl.identifier)),
            pdecl);

    TypeId result_type = TypeId::null_id();

    bool has_type = !pdecl.type.is_null();
    bool has_init = !pdecl.default_initializer.is_null();

    if (!has_type && !has_init)
        return fail("parameter declaration without either a type annotation or a default "
                    "initializer is not allowed",
                    pdecl);

    if (has_type) {
        if (has_init) {
            check(pdecl.default_initializer, pdecl.type);
        }

        result_type = pdecl.type;
    } else {
        result_type = infer(pdecl.default_initializer);
    }

    NodeId decl_id = ctx.calculate_node_id(&pdecl, pdecl.kind());
    st_register(pdecl.identifier, decl_id);

    return result_type;
}

TypeId JLSema::visit_FunctionDecl(FunctionDecl& fn_decl) {
    if (fn_decl.identifier.is_null())
        return internal_error("function declaration with null as the identifier symbol", fn_decl);

    return tpool.func_td(fn_decl.identifier);
}

TypeId JLSema::visit_MethodDecl(MethodDecl& method) {
    if (!ctx.isa<CompoundExpr>(method.body))
        return fail("method declaration with non-compound expression as a body is not allowed",
                    method);

    NodeId method_id = ctx.calculate_node_id(&method, method.kind());

    // NOTE:
    // reasoning for param initializer handling would be too complicated to describe here
    // see my thesis for an in-depth explanation with examples.
    // short version: because Julia generates a separate wrapper method for each possible arity, and
    // default initializers can be arbitrary expressions (with their own assignments/declarations),
    // how symbol visibility is handled across them can get tricky

    std::vector<TypeId> param_types{};
    param_types.reserve(method.param_decls.size());

    std::vector<std::reference_wrapper<ParamDecl>> param_decls{};
    param_decls.reserve(method.param_decls.size());

    for (NodeId param : method.param_decls) {
        auto* pdecl = ctx.get_and_dyn_cast<ParamDecl>(param);

        if (pdecl == nullptr)
            return internal_error("invalid node kind in param decl list of a method decl", method);

        param_decls.emplace_back(*pdecl);
    }

    assert(param_decls.size() == method.param_decls.size());

    size_t first_init_idx = 0;
    for (ParamDecl& pdecl : param_decls) {
        if (!pdecl.default_initializer.is_null())
            break;

        // ? pdecl->type might be null here. that's okay, as long as we can infer it later
        param_types.push_back(pdecl.type);
        first_init_idx++;
    }

    assert(first_init_idx <= param_decls.size());

    // this handles iterating over all possible arities of the method
    for (size_t i = first_init_idx; i < param_decls.size(); i++) {
        ParamDecl& pdecl = param_decls[i];

        if (pdecl.default_initializer.is_null())
            return fail("parameter without a default initializer following a default initialized "
                        "parameter in method signature",
                        pdecl);

        {
            std::vector<NodeId> dummy_method_body{};
            dummy_method_body.reserve(method.param_decls.size() - i + 1);

            for (size_t j = i; j < method.param_decls.size(); j++) {
                auto* pdecl_j = ctx.get_and_dyn_cast<ParamDecl>(method.param_decls[i]);
                if (pdecl_j == nullptr || pdecl_j->default_initializer.is_null())
                    continue;

                // eval of initializer
                dummy_method_body.push_back(pdecl_j->default_initializer);
            }

            CompoundExpr dummy_wrapper{method.location, std::move(dummy_method_body)};

            // hard scope because of generated wrapper functions
            ScopeRAII dummy_scope{*this, ScopeKind::Hard, dummy_wrapper, param_decls};

            // FEATURE: allow method decl to resolve to different signatures for different arities

            for (size_t j = i; j < method.param_decls.size(); j++) {
                if (param_types.size() < j)
                    param_types.push_back(infer(method.param_decls[j]));
                else if (param_types[j].is_null())
                    param_types[j] = infer(method.param_decls[j]);
                else
                    check(method.param_decls[j], param_types[j]);

                // TODO: allow param type infer from body
                if (param_types[j].is_null())
                    return fail(
                        std::format(
                            "couldn't infer type for method parameter '{}'. "
                            "Currently, a parameter must either have an explicit type annotation, "
                            "or its type must be inferrable from its default initializer.",
                            ctx.get_sym(param_decls[j].get().identifier)),
                        param_decls[j]);
            }
        }
    }

    // inferred return types don't support
    if (method.ret_type.is_null())
        visit_method_body(method);
    else
        current_scope().defer_method_body_visit(method_id);

    return tpool.method_td(method.ret_type, std::move(param_types));
}

void JLSema::visit_method_body(MethodDecl& method) {
    NodeId method_id = ctx.calculate_node_id(&method, method.kind());

    std::vector<std::reference_wrapper<ParamDecl>> param_decls;
    param_decls.reserve(method.param_decls.size());

    for (NodeId pdecl_id : method.param_decls) {
        auto* pdecl = ctx.get_and_dyn_cast<ParamDecl>(pdecl_id);
        assert(pdecl != nullptr);
        param_decls.emplace_back(*pdecl);
    }

    // fn scope
    ScopeRAII fn_scope{*this, ScopeKind::Hard, method.body, param_decls};
    if (!fn_scope.sym_res_successful()) {
        fail("symbol resolution pass failed in function scope of a method declaration", method);
        return;
    }

    st_register(method.identifier, method_id);

    // this registers into function scope (dummy_scope swallows symbols from param decl visitor)
    for (ParamDecl& pdecl : param_decls)
        st_register(pdecl.identifier, ctx.calculate_node_id(&pdecl, pdecl.kind()));

    TypeId prev_ret = current_fn_ret;
    current_fn_ret  = method.ret_type;

    MethodDecl* prev_method = current_method;
    current_method          = &method;

    if (!current_fn_ret.is_null()) {
        check(method.body, current_fn_ret);
    } else {
        // returns in body infer into, or check against current_fn_ret
        // last expr also behaves similarly
        TypeId body_inf = infer(method.body);

        if (current_fn_ret.is_null())
            current_fn_ret = body_inf;

        if (current_fn_ret != body_inf) {
            fail("type mismatch between function body's inferred return type, and "
                 "function's inferred return type based on return statements",
                 method);
            return;
        }

        method.ret_type = current_fn_ret;
    }

    current_method = prev_method;
    current_fn_ret = prev_ret;

    // add to fn decl, or create one if it doesnt exist yet

    NodeId fn_decl_id = find_local_sym(method.identifier);

    if (fn_decl_id.is_null()) {
        fn_decl_id = ctx.emplace_node<FunctionDecl>(method.location, method.identifier,
                                                    std::vector{method_id})
                         .first;

        // global + fn scope
        assert(scopes.size() >= 2);

        std::vector<NodeId>& def_scope_body = scopes[scopes.size() - 2].body.body;

        auto it = std::find(def_scope_body.begin(), def_scope_body.end(), method_id);

        if (it == def_scope_body.end()) {
            internal_error("failed to find MethodDecl node in the containing scope's body", method);
            return;
        }

        def_scope_body.insert(it, fn_decl_id);
    }

    auto* fn_decl = ctx.get_and_dyn_cast<FunctionDecl>(fn_decl_id);
    assert(fn_decl != nullptr);

    fn_decl->methods.push_back(method_id);
}

// TODO: structs

TypeId JLSema::visit_FieldDecl(FieldDecl& fdecl) {
    return fail("Structs are not supported yet", fdecl);
}

TypeId JLSema::visit_StructDecl(StructDecl& sdecl) {
    return fail("Structs are not supported yet", sdecl);
}

TypeId JLSema::visit_CompoundExpr(CompoundExpr& cmpd) {
    if (cmpd.body.empty())
        return ctx.jl_Nothing_t();

    TypeId result_type = TypeId::null_id();

    for (size_t i = 0; i < cmpd.body.size() - 1; i++)
        infer(cmpd.body[i]);

    if (is_checking()) {
        check(cmpd.body.back());
        result_type = expected_type;
    } else {
        result_type = infer(cmpd.body.back());
    }

    return result_type;
}

#define DEFINE_LIT(type)                                                                           \
    TypeId JLSema::visit_##type##Literal([[maybe_unused]] type##Literal& lit) {                    \
        return ctx.jl_##type##_t();                                                                \
    }

// TODO: value checks for some of these
DEFINE_LIT(Bool)
DEFINE_LIT(Int32)
DEFINE_LIT(Int64)
DEFINE_LIT(UInt8)
DEFINE_LIT(UInt16)
DEFINE_LIT(UInt32)
DEFINE_LIT(UInt64)
DEFINE_LIT(UInt128)
DEFINE_LIT(Float32)
DEFINE_LIT(Float64)
DEFINE_LIT(String)

#undef DEFINE_LIT

TypeId JLSema::visit_SymbolLiteral(SymbolLiteral& sym) {
    return internal_error(
        "sema pass reached a symbol literal leaf node, which should never happen.", sym);
}

TypeId JLSema::visit_NothingLiteral([[maybe_unused]] NothingLiteral& lit) {
    return ctx.jl_Nothing_t();
}

TypeId JLSema::visit_OpaqueNode(OpaqueNode& opaq) {
    return fail("opaque value found in source AST.", opaq);
}

TypeId JLSema::visit_GlobalRef(GlobalRef& gref) {
    return fail("global ref found in source AST.", gref);
}

TypeId JLSema::visit_DeclRefExpr(DeclRefExpr& dre) {
    auto* inner = ctx.get_node(dre.decl);
    if (inner == nullptr)
        return internal_error("declaration reference expression points to null", dre);

    // already resolved state
    if (auto* decl = dyn_cast<Decl>(inner)) {
        return decl->type;
    }

    if (auto* sym = dyn_cast<SymbolLiteral>(inner)) {
        auto maybe_bt = binding_of(sym->value);

        if (!maybe_bt.has_value())
            return internal_error(
                std::format("symbol resolution pass failed to infer binding type for "
                            "symbol '{}' in a declaration",
                            ctx.get_sym(sym->value)),
                *sym);

        bool is_captured   = *maybe_bt == BindingType::Captured;
        NodeId reffed_decl = find_sym(sym->value);

        if (reffed_decl.is_null()) {
            if (is_captured)
                return fail(std::format("forward capture of symbol '{}' in method without explicit "
                                        "return type. To use forward captures in a method, it's "
                                        "required to specify the method's return type explicitly.",
                                        ctx.get_sym(sym->value)),
                            dre);

            return fail(std::format("use of undeclared or uninitialized symbol '{}'",
                                    ctx.get_sym(sym->value)),
                        dre);
        }

        auto* decl = ctx.get_and_dyn_cast<Decl>(reffed_decl);
        if (decl == nullptr)
            return internal_error("non-declaration node in symbol table", dre);

        dre.decl = reffed_decl;

        return decl->type;
    }

    if (auto* gref = dyn_cast<GlobalRef>(inner))
        return fail("global refs are currently not supported", *gref);

    return internal_error("declaration reference expression points to invalid node kind", dre);
}

TypeId JLSema::visit_Assignment(Assignment& assign) {
    Expr* lhs = ctx.get_node(assign.target);

    if (lhs == nullptr)
        return internal_error("invalid assignment lhs", assign);

    // ! TODO: type might be function type
    // TODO: use decl's initializer instead of decl-and-assign
    if (auto* dre = dyn_cast<DeclRefExpr>(lhs)) {
        Expr* lhs_target = ctx.get_node(dre->decl);

        // uninitialized symbol
        // infer rhs -> define decl from binding info of sym res pass
        if (auto* sym = dyn_cast<SymbolLiteral>(lhs_target)) {
            NodeId decl_id = find_sym(sym->value);

            if (decl_id.is_null()) {
                TypeId inf_type = infer(assign.value);

                auto maybe_bt = binding_of(sym->value);

                if (!maybe_bt.has_value())
                    return internal_error(
                        std::format("symbol resolution pass failed to infer binding "
                                    "type for symbol '{}' in assignment lhs",
                                    ctx.get_sym(sym->value)),
                        assign);

                BindingType bt = *maybe_bt;

                if (bt == BindingType::Captured) {
                    // TODO:
                    // build capture sema into MethodDecl, allow generating captures-as-args
                    // that could, in turn, allow this to be handled
                    return fail(
                        std::format(
                            "assignment to captured symbol before definition for symbol '{}'. "
                            "Currently, all variables must be assigned before they can be captured "
                            "by an inner function.",
                            ctx.get_sym(sym->value)),
                        assign);
                }

                auto [new_decl_id, new_decl_ptr] =
                    ctx.emplace_node<VarDecl>(sym->location, sym->value, inf_type, bt_to_st(bt));
                new_decl_ptr->type = inf_type;

                // if (bt == BindingType::Global)
                //     global_scope().st_add_sym(sym->value, new_decl_id);
                // else
                //     st_register(sym->value, new_decl_id);
                infer(new_decl_id);

                dre->decl = new_decl_id;

                // tie back traversal into the general visitor logic
                infer(assign.target);

                return inf_type;
            }

            // already initialized symbol
            // infer lhs -> check rhs
            dre->decl = decl_id;
        }
    }

    TypeId target_type = infer(assign.target);
    check(assign.value, target_type);

    return target_type;
}

// TODO: builtin fns, Base.return_types
TypeId JLSema::visit_FunctionCall(FunctionCall& fn_call) {
    infer(fn_call.target_fn);

    FunctionDecl* fn_decl = nullptr;
    if (auto* fn_dre = ctx.get_and_dyn_cast<DeclRefExpr>(fn_call.target_fn)) {
        if (fn_dre->decl.is_null())
            return internal_error("empty declaration in function call's target", fn_call);

        Expr* decl_expr = ctx.get_node(fn_dre->decl);
        assert(decl_expr != nullptr);

        auto* decl = dyn_cast<Decl>(decl_expr);
        assert(decl != nullptr);

        fn_decl = dyn_cast<FunctionDecl>(decl_expr);

        if (fn_decl == nullptr)
            return fail(
                std::format("call to non-function symbol '{}'", ctx.get_sym(decl->identifier)),
                fn_call);
    } else {
        return internal_error("unexpected node kind as function call's target", fn_call);
    }

    std::vector<TypeId> arg_types{};
    arg_types.reserve(fn_call.args.size());

    for (NodeId arg : fn_call.args) {
        if (arg.is_null())
            return internal_error("null node as argument to function call", fn_call);

        TypeId arg_type = infer(arg);

        if (arg_type.is_null()) {
            Expr* arg_expr = ctx.get_node(arg);
            assert(arg_expr != nullptr);

            return fail("cannot infer static type for argument in function call", *arg_expr);
        }

        arg_types.push_back(arg_type);
    }

    assert(arg_types.size() == fn_call.args.size());

    MethodDecl* target_method = nullptr;
    for (NodeId method_id : fn_decl->methods) {
        auto* mdecl = ctx.get_and_dyn_cast<MethodDecl>(method_id);

        if (mdecl == nullptr)
            return internal_error(std::format("non-method-declaration node in method list of "
                                              "function declaration for symbol '{}'",
                                              ctx.get_sym(fn_decl->identifier)),
                                  *fn_decl);

        if (mdecl->param_decls.size() != arg_types.size())
            continue;

        auto arg_it    = arg_types.begin();
        bool sig_match = true;
        for (NodeId param_id : mdecl->param_decls) {
            assert(arg_it != arg_types.end());

            auto* pdecl = ctx.get_and_dyn_cast<ParamDecl>(param_id);

            if (pdecl == nullptr)
                return internal_error(
                    std::format("non-parameter-declaration node in parameter list of "
                                "method declaration for symbol '{}'",
                                ctx.get_sym(mdecl->identifier)),
                    *mdecl);

            // TODO: subsumption
            if (*arg_it != pdecl->type) {
                sig_match = false;
                break;
            }

            arg_it++;
        }

        if (sig_match) {
            if (mdecl == current_method && mdecl->ret_type.is_null())
                return fail(
                    std::format("recursion on method with implicit return type is not allowed"),
                    fn_call);

            target_method = mdecl;
            break;
        }
    }

    if (target_method == nullptr)
        return fail(
            std::format("no method matches inferred argument types for function call to '{}'",
                        ctx.get_sym(fn_decl->identifier)),
            fn_call);

    return target_method->ret_type;
}

TypeId JLSema::visit_IfExpr(IfExpr& if_) {
    check(if_.condition, ctx.jl_Bool_t());

    if (if_.false_branch.is_null()) {
        infer(if_.true_branch);

        return ctx.jl_Nothing_t();
    }

    if (is_checking()) {
        check(if_.true_branch);
        check(if_.false_branch);

        return expected_type;
    }

    TypeId inf_tb = infer(if_.true_branch);
    TypeId inf_fb = infer(if_.false_branch);

    if (inf_tb == inf_fb)
        return inf_tb;

    return ctx.jl_Nothing_t();
}

TypeId JLSema::visit_WhileExpr(WhileExpr& while_) {
    if (while_.condition.is_null())
        return fail("empty condition in while expression", while_);

    auto* cmpd = ctx.get_and_dyn_cast<CompoundExpr>(while_.body);
    if (cmpd == nullptr)
        return fail("non-compound-expression node used as a while expression's body", while_);

    CompoundExpr wrapper_cmpd{while_.location, {while_.condition, while_.body}};
    {
        ScopeRAII scope{*this, ScopeKind::Soft, wrapper_cmpd};
        if (!scope.sym_res_successful())
            return fail("symbol resolution pass failed for body of while expression", while_);

        check(while_.condition, ctx.jl_Bool_t());

        infer(while_.body);
    }

    return ctx.jl_Nothing_t();
}

TypeId JLSema::visit_ReturnStmt(ReturnStmt& ret) {
    if (current_method == nullptr)
        return fail("return statement outside of method body", ret);

    if (!ret.inner.is_null()) {
        if (current_fn_ret.is_null())
            current_fn_ret = infer(ret.inner);
        else
            check(ret.inner, current_fn_ret);
    } else if (!current_fn_ret.is_null()) {
        return fail(std::format("empty return stmt in function expected to return {}",
                                type_str(current_fn_ret)),
                    ret);
    }

    return TypePool::void_td();
}

TypeId JLSema::visit_ContinueStmt([[maybe_unused]] ContinueStmt& cont) {
    return TypePool::void_td();
}

TypeId JLSema::visit_BreakStmt([[maybe_unused]] BreakStmt& brk) {
    return TypePool::void_td();
}

} // namespace stc::jl
