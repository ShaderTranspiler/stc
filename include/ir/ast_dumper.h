#pragma once

#include <iostream>

#include "ir/ast_visitor.h"

namespace stc::ir {

class ASTDumper : public ASTVisitor<ASTDumper> {
private:
    std::ostream& out;
    size_t indent_level = 0U;

public:
    explicit ASTDumper(ASTCtx& ctx, std::ostream& out)
        : ASTVisitor{ctx}, out{out} {}

    void pre_visit_stmt(StmtId stmt);
    void pre_visit_decl(DeclId decl);

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(void, type)
        #include "ir/node_defs/decl.def"
        #include "ir/node_defs/stmt.def"
    #undef X
    // clang-format on

private:
    std::string type_str(TypeId type_id) const;
    std::string indent() const;
    void inc_indent(size_t level = STC_DUMP_INDENT);
    void dec_indent(size_t level = STC_DUMP_INDENT);
};

static_assert(ASTVisitorImpl<ASTDumper, void>);

} // namespace stc::ir
