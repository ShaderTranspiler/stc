#pragma once

#include "common/bump_arena.h"
#include "ir/ast.h"
#include "ir/type_pool.h"

namespace stc::ir {

struct ASTCtx {
private:
    BumpArena<NodeId> node_arena;            // currently: BumpArena32
    BumpArena<SrcLocationId> src_info_arena; // currently: BumpArena32
    BumpArena<TypeId> type_arena;            // currently: BumpArena16

public:
    TypePool type_pool;
    SrcInfoManager src_info_manager;

    explicit ASTCtx()
        : node_arena{128 * 1024},
          src_info_arena{128 * 1024},
          type_arena{65535},
          type_pool{type_arena},
          src_info_manager{src_info_arena} {}

    template <typename T, typename... Args>
    std::pair<uint32_t, T*> alloc_node(Args&&... args) {
        return node_arena.emplace<T>(std::forward<Args>(args)...);
    }
};

}; // namespace stc::ir
