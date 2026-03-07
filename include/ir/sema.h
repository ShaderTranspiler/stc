#pragma once

#include "ir/ast.h"
#include "ir/ast_context.h"

namespace stc::ir {

class Sema final {
    const ASTCtx ctx;

    explicit Sema() {}
};

} // namespace stc::ir