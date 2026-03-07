#include "ir/ast_context.h"

namespace stc::ir {

Stmt* ASTCtx::get_stmt(StmtId id) const {
    return static_cast<Stmt*>(stmt_arena.get_ptr(id));
}

Decl* ASTCtx::get_decl(DeclId id) const {
    return static_cast<Decl*>(decl_arena.get_ptr(id));
}

} // namespace stc::ir