#pragma once

#include "ast/context.h"
#include "frontend/jl/ast.h"
#include "frontend/jl/rt/env.h"

#include <ostream>

namespace stc::jl {

using types::BuiltinTD;

struct Expr;
enum class NodeKind : uint8_t;

enum class BuiltinTypeKind : uint8_t { Nothing, String, Symbol };

class JLCtx : public ASTCtx<NodeId, Expr> {
    using Base = ASTCtx<NodeId, Expr>;

public:
    rt::JuliaRTEnv jl_env;

    // clang-format off
    #define X(type) types::TypeId type() const { return _##type; }
        #include "frontend/jl/node_defs/types.def"
    #undef X
    // clang-format on

    explicit JLCtx(std::string_view julia_glm_path, NodeId::id_type initial_node_arena_size,
                   SrcLocationId::id_type initial_src_info_arena_size,
                   TypeId::id_type initial_type_arena_size,
                   SymbolId::id_type initial_symbol_arena_size,
                   QualId::id_type initial_qualifier_arena_size,
                   const TargetInfo* target_info = nullptr)
        : ASTCtx{initial_node_arena_size,
                 initial_src_info_arena_size,
                 initial_type_arena_size,
                 initial_symbol_arena_size,
                 initial_qualifier_arena_size,
                 {{BuiltinTypeKind::Nothing}, {BuiltinTypeKind::String}, {BuiltinTypeKind::Symbol}},
                 target_info,
                 "Julia Node Pool"},
          jl_env{julia_glm_path} {
        init_jl_types();
    }

    JLCtx(const JLCtx&)            = delete;
    JLCtx& operator=(const JLCtx&) = delete;

    // CLEANUP: these are currently only disabled because RT module cache stores refs
    // this does not affect partial move semantics, so it's not a priority
    JLCtx(JLCtx&&)            = delete;
    JLCtx& operator=(JLCtx&&) = delete;

    ~JLCtx() = default;

protected:
    template <typename T, typename U>
    explicit JLCtx(ASTCtx<T, U>&& other, NodeId::id_type initial_node_arena_size)
        : Base{std::move(other), initial_node_arena_size} {
        init_jl_types();
    }

public:
    template <typename T, typename U>
    [[nodiscard]] static JLCtx move_pools_from(ASTCtx<T, U>&& other,
                                               NodeId::id_type initial_node_arena_size) {
        return JLCtx{std::move(other), initial_node_arena_size};
    }

    // safe to run on partially-moved contexts
    void dump(Expr& subtree_root, Expr& ast_root, DumpVerbosity verbosity_level, std::ostream& out,
              const TypePool& type_pool_override, const SymbolPool& sym_pool_override,
              const TranspilerConfig& config_override) const;

    // safe to run on partially-moved contexts
    void dump(NodeId subtree_root_id, NodeId ast_root_id, DumpVerbosity verbosity_level,
              std::ostream& out, const TypePool& type_pool_override,
              const SymbolPool& sym_pool_override, const TranspilerConfig& config_override) const {
        dump(get_expr_or_fail(subtree_root_id), get_expr_or_fail(ast_root_id), verbosity_level, out,
             type_pool_override, sym_pool_override, config_override);
    }

    // only safe to run on not-partially-moved contexts
    void dump(Expr& subtree_root, Expr& ast_root, DumpVerbosity verbosity_level,
              std::ostream& out) const {
        dump(subtree_root, ast_root, verbosity_level, out, type_pool, sym_pool, config);
    }

    // only safe to run on not-partially-moved contexts
    void dump(NodeId subtree_root_id, NodeId ast_root_id, DumpVerbosity verbosity_level,
              std::ostream& out) const {
        dump(get_expr_or_fail(subtree_root_id), get_expr_or_fail(ast_root_id), verbosity_level,
             out);
    }

private:
    // adds Julia type not already present and assigns them to the _<type name> members
    // if types are already inside the pool and the members, this is a no-op
    void init_jl_types();

    Expr& get_expr_or_fail(NodeId id) const {
        if (id.is_null())
            throw std::logic_error{"Mandatory node argument is null id"};

        Expr* expr = get_node(id);
        if (expr == nullptr)
            throw std::logic_error{
                "Mandatory node given id that doesn't point into the context's node pool"};

        return *expr;
    }

    // clang-format off
    #define X(type) types::TypeId _##type = types::TypeId::null_id();
        #include "frontend/jl/node_defs/types.def"
    #undef X
    // clang-format on
};

} // namespace stc::jl
