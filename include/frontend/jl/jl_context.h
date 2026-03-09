#pragma once

#include "ast/ast_context.h"
#include "frontend/jl/ast.h"

namespace stc::jl {

class JLCtx : public ASTCtx<NodeId, Expr, NodeKind> {};

} // namespace stc::jl
