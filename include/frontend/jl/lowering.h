#pragma once

#include "frontend/jl/visitor.h"
#include "sir/context.h"

#include <algorithm>

namespace stc::jl {

// ! IMPORTANT !
// the lowering visitor performs a "partial move" upon construction. this means that it moves source
// info, type and symbol pools from the Julia context to a newly created SIR context. this has two
// important implications:
// a) the julia context passed in should no longer be used externally (similarly to a regular move)
// b) inside the visitor code, nodes can be safely looked up from the original Julia context (ctx),
//    but other resources should be looked up from the SIR context (sir_ctx)
//    partial move ensures that non-node ID-s in the Julia AST can be used for lookup into the new
//    SIR context without problem

class JLLoweringVisitor final : public JLVisitor<JLLoweringVisitor, JLCtx, sir::NodeId> {
    using SIRNodeId = sir::NodeId;

    std::unordered_map<Decl*, SIRNodeId> decl_map{};
    bool success   = true;
    bool in_method = false;

public:
    explicit JLLoweringVisitor(JLCtx&& ctx)
        : JLVisitor{ctx}, sir_ctx{sir::SIRCtx::move_pools_from(std::move(ctx))} {

        sym_plus     = sir_ctx.sym_pool.get_id("+");
        sym_minus    = sir_ctx.sym_pool.get_id("-");
        sym_asterisk = sir_ctx.sym_pool.get_id("*");
    }

    bool pre_visit_ptr(Expr* expr);
    SIRNodeId visit_default_case();

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(SIRNodeId, type)
        #include "frontend/jl/node_defs/all_nodes.def"
    #undef X
    // clang-format on

    SIRNodeId lower(NodeId global_cmpd_id);

    bool successful() const { return success; }
    sir::SIRCtx sir_ctx;

private:
    template <typename T, typename... Args>
    SIRNodeId emplace_node(Args&&... args) {
        return sir_ctx.template emplace_node<T>(std::forward<Args>(args)...).first;
    }

    template <typename T, typename... Args>
    SIRNodeId emplace_decl(Decl* src_decl, Args&&... args) {
        if (src_decl == nullptr)
            throw std::logic_error{"Source declaration cannot be null"};

        if (decl_map.contains(src_decl))
            throw std::logic_error{std::format("Trying to lower declaration for symbol '{}', but "
                                               "it already has a lowered match",
                                               sir_ctx.get_sym(src_decl->identifier))};

        SIRNodeId id = sir_ctx.template emplace_node<T>(std::forward<Args>(args)...).first;
        decl_map.try_emplace(src_decl, id);

        return id;
    }

    TypeId lower_type(TypeId type) {
        if (type == ctx.jl_Nothing_t())
            return sir_ctx.type_pool.void_td();

        return type;
    }

    void swap_lower_type(TypeId& type) { type = lower_type(type); }

    SIRNodeId fail(std::string_view msg);
    SIRNodeId internal_error(std::string_view msg);
    SIRNodeId visit_and_check(NodeId id);

    // skips id-lookup roundtrip for nodes that have already been looked up
    SIRNodeId visit_ptr(Expr* node);

    SymbolId sym_plus, sym_minus, sym_asterisk;
};

} // namespace stc::jl