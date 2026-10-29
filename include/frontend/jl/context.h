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

    explicit JLCtx(std::string_view juliaglm_path = "Main.JuliaGLM",
                   const TargetInfo* target_info = nullptr, NodeId::id_type node_arena_kb = 128U,
                   SrcLocationId::id_type src_info_arena_kb = 128U,
                   TypeId::id_type type_arena_kb = 32U, SymbolId::id_type sym_arena_kb = 64U,
                   QualId::id_type qual_arena_kb = 16U)
        : ASTCtx{{{BuiltinTypeKind::Nothing}, {BuiltinTypeKind::String}, {BuiltinTypeKind::Symbol}},
                 target_info,
                 node_arena_kb,
                 src_info_arena_kb,
                 type_arena_kb,
                 sym_arena_kb,
                 qual_arena_kb},
          jl_env{juliaglm_path} {

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
    explicit JLCtx(ASTCtx<T, U>&& other, NodeId::id_type node_arena_kb = 128U)
        : Base{std::move(other), node_arena_kb} {
        init_jl_types();
    }

public:
    template <typename T, typename U>
    [[nodiscard]] static JLCtx move_pools_from(ASTCtx<T, U>&& other,
                                               NodeId::id_type node_arena_kb) {
        return JLCtx{std::move(other), node_arena_kb};
    }

    // safe to run on partially-moved contexts
    void dump(Expr& subtree_root, Expr& ast_root, DumpVerbosity verbosity_level, std::ostream& out,
              const TypePool& type_pool, const SymbolPool& sym_pool,
              const TranspilerConfig& config) const;

    // safe to run on partially-moved contexts
    void dump(NodeId subtree_root_id, NodeId ast_root_id, DumpVerbosity verbosity_level,
              std::ostream& out, const TypePool& type_pool, const SymbolPool& sym_pool,
              const TranspilerConfig& config) const {
        dump(get_expr_or_fail(subtree_root_id), get_expr_or_fail(ast_root_id), verbosity_level, out,
             type_pool, sym_pool, config);
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
