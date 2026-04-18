#pragma once

#include "julia_guard.h"

#include "frontend/jl/context.h"
#include "types/type_visitor.h"

namespace stc::jl {

using namespace stc::types;

class TypeToJLVisitor : public TypeVisitor<TypeToJLVisitor, jl_datatype_t*> {
    friend class TypeVisitor<TypeToJLVisitor, jl_datatype_t*>;

    const JLCtx& ctx;
    const rt::JuliaTypeCache& type_cache;

public:
    explicit TypeToJLVisitor(const JLCtx& ctx)
        : TypeVisitor{ctx.type_pool}, ctx{ctx}, type_cache{ctx.jl_env.type_cache} {}

private:
    jl_datatype_t* visit_null_id();

    STC_TYPE_VISITOR_DECLS(jl_datatype_t*)
};

TypeId parse_jl_type(jl_datatype_t* dt, JLCtx& ctx);

} // namespace stc::jl
