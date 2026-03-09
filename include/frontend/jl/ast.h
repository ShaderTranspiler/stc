#pragma once

#include "common/src_info.h"
#include "common/utils.h"
#include "types/types.h"

#define SAME_NODE_T_DEF(Kind)                                                                      \
    static bool same_node_t(const NodeBase* node) {                                                \
        return node->kind() == (Kind);                                                             \
    }

namespace stc::jl {

using namespace stc::types;

struct NodeId : StrongId<uint32_t> {
    using StrongId::StrongId;

    bool is_null() const { return *this == null_id(); }

    static constexpr NodeId null_id() { return 0U; }
};

// clang-format off
enum class NodeKind : uint8_t {
    InvalidKind,

    #define X(type, kind) kind,
        #include "frontend/jl/node_defs/all_nodes.def"
    #undef X
};
// clang-format on

struct Expr {
    SrcLocationId location;
    TypeId type;
    uint16_t _node_storage;

    explicit Expr(SrcLocationId location, TypeId type, uint16_t node_storage = 0U)
        : location{location}, type{type}, _node_storage{node_storage} {}
};

}; // namespace stc::jl

#undef SAME_NODE_T_DEF
