#pragma once

#include "frontend/jl/ast.h"
#include "frontend/jl/context.h"
#include "frontend/jl/parser_caches.h"
#include "julia_wrapper.h"
#include "sir/context.h"

#define PARSER_DECL(name) NodeId parse_##name(jl_expr_t* expr, size_t nargs)

namespace stc::jl {

class JLParser {
public:
    explicit JLParser()
        : ctx{} {

        std::ignore = ctx.src_info_pool.get_file("dummy file");
        cur_loc     = ctx.src_info_pool.get_location(1, 1);
    }

    NodeId parse(jl_value_t* node);
    NodeId parse_expr(jl_expr_t* expr);
    NodeId parse_code(std::string_view code);

    PARSER_DECL(var_decl);
    PARSER_DECL(assignment);
    PARSER_DECL(block);
    PARSER_DECL(call);
    PARSER_DECL(if);
    PARSER_DECL(while);
    PARSER_DECL(return);

    [[nodiscard]] JLCtx&& steal_ctx() { return std::move(ctx); }

private:
    JLCtx ctx;
    SrcLocationId cur_loc;

    JLParserTypeCache type_cache{};
    JLParserSymbolCache sym_cache{};

    template <typename T, typename... Args>
    NodeId emplace_node(Args&&... args) {
        return ctx.emplace_node<T>(std::forward<Args>(args)...).first;
    }

    TypeId resolve_type(jl_value_t* type);
};

#undef PARSER_DECL

} // namespace stc::jl
