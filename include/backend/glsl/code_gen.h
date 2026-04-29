#pragma once

#include <sstream>

#include "backend/glsl/context.h"
#include "sir/visitor.h"

namespace stc::glsl {

using namespace stc::sir;

class GLSLCodeGenVisitor final : public SIRVisitor<GLSLCodeGenVisitor, GLSLCtx, void> {
    using Base = SIRVisitor<GLSLCodeGenVisitor, GLSLCtx, void>;

    std::stringstream out{};
    size_t indent_level = 0U;
    bool _success       = true;

    SymbolId sym_length;

public:
    explicit GLSLCodeGenVisitor(GLSLCtx& ctx)
        : SIRVisitor<GLSLCodeGenVisitor, GLSLCtx, void>{ctx} {

        sym_length = ctx.sym_pool.get_id("length");

        out << "#version " << ctx.config.target_version << "\n\n";
        out << "// THIS CODE WAS AUTO-GENERATED USING THE STC TRANSPILER LIBRARY\n\n";

        if (ctx.config.local_size_x != 0) {
            out << fmt::format(
                "layout(local_size_x = {}, local_size_y = {}, local_size_z = {}) in;\n\n",
                ctx.config.local_size_x, ctx.config.local_size_y, ctx.config.local_size_z);
        }
    }

    // ! this performs a move on the internal buffer
    std::string move_result() {
        out.flush();

        return std::move(out).str();
    }

    bool success() const { return _success; }

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(void, type)
        #include "sir/node_defs/all_nodes.def"
    #undef X
    // clang-format on

private:
    std::string indent() const;
};

} // namespace stc::glsl
