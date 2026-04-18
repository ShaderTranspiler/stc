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
    bool successful_gen = true;

public:
    explicit GLSLCodeGenVisitor(GLSLCtx& ctx)
        : SIRVisitor<GLSLCodeGenVisitor, GLSLCtx, void>{ctx} {

        // TODO: config for version
        out << "#version 460\n\n// THIS CODE WAS AUTO-GENERATED USING A JULIA TO GLSL "
               "TRANSPILER\n\n";
    }

    std::string result() {
        out.flush();
        return out.str();
    }
    bool successful() const { return successful_gen; }

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(void, type)
        #include "sir/node_defs/all_nodes.def"
    #undef X
    // clang-format on

private:
    std::string indent() const;
};

} // namespace stc::glsl
