#pragma once

#include "ast/context.hpp"
#include "sir/ast.hpp"

namespace stc::sir {

class SIRCtx : public ASTCtx<NodeId, NodeBase> {
public:
    explicit SIRCtx(NodeId::id_type initial_node_arena_size,
                    SrcLocationId::id_type initial_src_info_arena_size,
                    TypeId::id_type initial_type_arena_size,
                    SymbolId::id_type initial_symbol_arena_size,
                    QualId::id_type initial_qualifier_arena_size,
                    const std::vector<BuiltinTD>& type_builtins = std::vector<BuiltinTD>{},
                    const TargetInfo* target_info               = nullptr)
        : ASTCtx(initial_node_arena_size, initial_src_info_arena_size, initial_type_arena_size,
                 initial_symbol_arena_size, initial_qualifier_arena_size, type_builtins,
                 target_info, "SIR Node Pool") {}

    SIRCtx(const SIRCtx&)                = delete;
    SIRCtx& operator=(const SIRCtx&)     = delete;
    SIRCtx(SIRCtx&&) noexcept            = default;
    SIRCtx& operator=(SIRCtx&&) noexcept = default;

protected:
    template <typename T, typename U>
    explicit SIRCtx(ASTCtx<T, U>&& other, NodeId::id_type initial_node_arena_size)
        : ASTCtx{std::move(other), initial_node_arena_size} {
        node_pool_name = "SIR Node Pool";
    }

public:
    template <typename T, typename U>
    [[nodiscard]] static SIRCtx move_pools_from(ASTCtx<T, U>&& other,
                                                NodeId::id_type initial_node_arena_size) {
        return SIRCtx{std::move(other), initial_node_arena_size};
    }

    Decl* get_decl(NodeId id) const { return get_and_dyn_cast<Decl>(id); }
    Stmt* get_stmt(NodeId id) const { return get_and_dyn_cast<Stmt>(id); }
    Expr* get_expr(NodeId id) const { return get_and_dyn_cast<Expr>(id); }
};

}; // namespace stc::sir
