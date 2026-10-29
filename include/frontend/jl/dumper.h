#pragma once

#include <iostream>

#include "common/config.h"
#include "frontend/jl/visitor.h"

namespace stc::jl {

// ctx can be partially moved, use the constructor taking pools explicitly for this case
class JLDumper final : public JLVisitor<JLDumper, const JLCtx, void> {
private:
    const TypePool& type_pool;
    const SymbolPool& sym_pool;
    const TranspilerConfig& config;

    std::ostream& out;
    size_t indent_level = 0U;

public:
    static constexpr const char* PASS_NAME = "Julia Dumper";

    // constructor for not-partially-moved contexts
    explicit JLDumper(const JLCtx& ctx, std::ostream& out)
        : JLVisitor{ctx},
          type_pool{ctx.type_pool},
          sym_pool{ctx.sym_pool},
          config{ctx.config},
          out{out} {}

    // constructor for partially-moved contexts
    explicit JLDumper(const JLCtx& ctx, const TypePool& type_pool, const SymbolPool& sym_pool,
                      const TranspilerConfig& config, std::ostream& out)
        : JLVisitor{ctx}, type_pool{type_pool}, sym_pool{sym_pool}, config{config}, out{out} {}

    // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method)
    bool pre_visit_id(NodeId node);
    // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method)
    bool pre_visit_ptr(Expr* expr);

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(void, type)
        #include "frontend/jl/node_defs/all_nodes.def"
    #undef X
    // clang-format on

private:
    std::string type_str(TypeId type_id) const;
    std::string_view sym(SymbolId sym_id) const;
    std::string indent() const;
    void inc_indent();
    void dec_indent();

    void dump_with_label(std::string_view label, NodeId node);
    void dump_with_label(std::string_view label, const std::vector<NodeId>& nodes);
};

static_assert(CJLVisitorImpl<JLDumper, void>);

} // namespace stc::jl
