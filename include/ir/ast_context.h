#pragma once

#include "common/bump_arena.h"
#include "ir/ast.h"
#include "ir/type_pool.h"

namespace stc::ir {

struct ASTCtx {
private:
    BumpArena<StmtId::id_type> stmt_arena;            // currently: BumpArena32
    BumpArena<DeclId::id_type> decl_arena;            // currently: BumpArena16
    BumpArena<SrcLocationId::id_type> src_info_arena; // currently: BumpArena32
    BumpArena<TypeId::id_type> type_arena;            // currently: BumpArena16

public:
    TypePool type_pool;
    SrcInfoManager src_info_manager;

    explicit ASTCtx()
        : stmt_arena{128 * 1024},
          src_info_arena{128 * 1024},
          type_arena{32 * 1024},
          type_pool{type_arena},
          src_info_manager{src_info_arena} {}

    template <typename T, typename... Args>
    requires std::derived_from<T, Stmt>
    std::pair<StmtId, T*> alloc_stmt(Args&&... args) {
        return stmt_arena.emplace<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    requires std::derived_from<T, Decl>
    std::pair<DeclId, T*> alloc_decl(Args&&... args) {
        return decl_arena.emplace<T>(std::forward<Args>(args)...);
    }

    Stmt* get_stmt(StmtId id) const;
    Decl* get_decl(DeclId id) const;
};

}; // namespace stc::ir
