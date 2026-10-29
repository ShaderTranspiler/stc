#pragma once

#include "types/type_visitor.h"

namespace stc::types {

class TypeToStringVisitor : public TypeVisitor<TypeToStringVisitor, std::string> {
    friend class TypeVisitor<TypeToStringVisitor, std::string>;

    const SymbolPool& sym_pool;

public:
    explicit TypeToStringVisitor(const TypePool& type_pool, const SymbolPool& sym_pool)
        : TypeVisitor{type_pool}, sym_pool{sym_pool} {}

    using TypeVisitor::dispatch;
    std::string dispatch(const TypeDescriptor& td);

    std::string visit_null_id();

private:
    STC_TYPE_VISITOR_DECLS(std::string)
};

inline std::string type_to_string(TDVariantType type, const TypePool& type_pool,
                                  const SymbolPool& sym_pool) {
    return TypeToStringVisitor{type_pool, sym_pool}.dispatch(type);
}

inline std::string type_to_string(const TypeDescriptor& td, const TypePool& type_pool,
                                  const SymbolPool& sym_pool) {
    return TypeToStringVisitor{type_pool, sym_pool}.dispatch(td);
}

inline std::string type_to_string(TypeId id, const TypePool& type_pool,
                                  const SymbolPool& sym_pool) {
    return TypeToStringVisitor{type_pool, sym_pool}.dispatch(id);
}

static_assert(CHasNullIdTypeVisitor<TypeToStringVisitor, std::string>,
              "no null case visitor defined for TypeToStringVisitor");

} // namespace stc::types
