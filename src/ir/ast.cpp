#include "ir/ast.h"

namespace {

/*

using OpKind = stc::ir::BinaryOp::OpKind;

std::string op_str(OpKind op) {
    switch (op) {
        case OpKind::add:
            return "+";

        case OpKind::sub:
            return "-";

        case OpKind::mul:
            return "*";

        case OpKind::div:
            return "/";

        case OpKind::pow:
            return "^";

        case OpKind::mod:
            return "%";
    }

    throw std::logic_error{"Unaccounted binary operator kind"};
}

std::string expr_short_str(const stc::ir::Expr* expr) {
    using namespace stc::ir;

    if (expr == nullptr)
        return "?";

    if (const auto* bool_lit = dynamic_cast<const BoolLiteral*>(expr))
        return bool_lit->value() ? "true" : "false";

    if (const auto* int_lit = dynamic_cast<const IntLiteral*>(expr))
        return int_lit->data;

    if (const auto* float_lit = dynamic_cast<const FloatLiteral*>(expr))
        return float_lit->data;

    return "?";
}
*/

} // namespace

namespace stc::ir {

// CLEANUP: move these into an AST visitor

/*
void Block::dump(size_t level, std::ostream& out) const {
out << indent(level) << "Block\n";

for (const StmtPtr& stmt : body)
    stmt->dump(level + 1, out);
}

void VarDecl::dump(size_t level, std::ostream& out) const {
// TODO: print type (requires ASTCtx)
out << indent(level) << "Variable Declaration (" << identifier << " : ?)";

if (initializer == nullptr) {
    out << '\n';
    return;
}

out << " with initializer:\n";
initializer->dump(level + 1, out);
}

void BoolLiteral::dump(size_t level, std::ostream& out) const {
out << indent(level) << "Bool Literal (" << expr_short_str(this) << ")\n";
}

void IntLiteral::dump(size_t level, std::ostream& out) const {
out << indent(level) << "Int Literal (" << data << ")\n";
}

void FloatLiteral::dump(size_t level, std::ostream& out) const {
out << indent(level) << "Float Literal (" << data << ")\n";
}

void VectorLiteral::dump(size_t level, std::ostream& out) const {
out << indent(level) << "Vector Literal\n";

for (const auto& el : components) {
    el->dump(level + 1, out);
}
}

void MatrixLiteral::dump(size_t level, std::ostream& out) const {
size_t width = data.size();
assert(width > 0);
size_t height = data[0].size();

out << indent(level) << "Matrix Literal (" << height << 'x' << width << "):\n";

// legends speak of this traversal producing cache hits once a decade
for (size_t i = 0; i < height; i++) {
    out << indent(level);

    for (size_t j = 0; j < width; j++) {
        out << expr_short_str(data[j][i].get());

        if (j != width - 1)
            out << ' ';
    }

    out << '\n';
}
}

void ArrayLiteral::dump(size_t level, std::ostream& out) const {
// TODO: print types (requires ASTCtx)
out << indent(level) << "Array Literal (?[" << elements.size() << "]: ";

for (size_t i = 0; i < elements.size(); i++) {
    out << expr_short_str(elements[i].get());

    if (i != elements.size() - 1)
        out << ", ";
}
}

void StructInstantiationLiteral::dump(size_t level, std::ostream& out) const {
// TODO: print struct type (requires ASTCtx)
out << indent(level) << "Struct Instantiation Literal (?):\n";

for (const auto& [field_name, field_val] : field_values) {
    out << field_name << " : ";
    field_val->dump(level + 1, out);
}
}

void BinaryOp::dump(size_t level, std::ostream& out) const {
out << indent(level) << "Binary Operation (" << op_str(op()) << ")\n";

if (lhs)
    lhs->dump(level + 1, out);

if (rhs)
    rhs->dump(level + 1, out);
}

void ExplicitCast::dump(size_t level, std::ostream& out) const {
// TODO: print cast type (requires ASTCtx)
out << indent(level) << "Explicit cast to ?\n";

base->dump(level + 1, out);
}

void IfStmt::dump(size_t level, std::ostream& out) const {
out << indent(level) << "If\nCondition:";
condition->dump(level + 1, out);
out << indent(level) << "Then:\n";
true_block->dump(level + 1, out);

if (false_block) {
    out << indent(level) << "Else:\n";
    false_block->dump(level + 1, out);
}
}

void ReturnStmt::dump(size_t level, std::ostream& out) const {
out << indent(level) << "Return\n";
ret_value->dump(level + 1, out);
}
*/

} // namespace stc::ir