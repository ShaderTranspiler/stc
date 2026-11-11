#pragma once

#include "backend/glsl/types.h"
#include "sir/context.h"

namespace stc::glsl {

class GLSLCtx : public sir::SIRCtx {
public:
    using sir::SIRCtx::SIRCtx;

    GLSLTypes types;

    explicit GLSLCtx(sir::NodeId::id_type initial_node_arena_size,
                     SrcLocationId::id_type initial_src_info_arena_size,
                     TypeId::id_type initial_type_arena_size,
                     SymbolId::id_type initial_symbol_arena_size,
                     QualId::id_type initial_qualifier_arena_size,
                     const TargetInfo* target_info = nullptr)
        : sir::SIRCtx{initial_node_arena_size,
                      initial_src_info_arena_size,
                      initial_type_arena_size,
                      initial_symbol_arena_size,
                      initial_qualifier_arena_size,
                      {},
                      target_info},
          types{type_pool} {}

    explicit GLSLCtx(SIRCtx&& other)
        : SIRCtx{std::move(other)}, types{type_pool} {}

protected:
    template <typename T, typename U>
    explicit GLSLCtx(ASTCtx<T, U>&& other, sir::NodeId::id_type initial_node_arena_size)
        : SIRCtx{std::move(other), initial_node_arena_size}, types{type_pool} {}

public:
    template <typename T, typename U>
    [[nodiscard]] static GLSLCtx move_pools_from(ASTCtx<T, U>&& other,
                                                 sir::NodeId::id_type initial_node_arena_size) {
        return GLSLCtx{std::move(other), initial_node_arena_size};
    }
};

} // namespace stc::glsl
