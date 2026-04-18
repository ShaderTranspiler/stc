#pragma once

#include "types/type_descriptors.h"
#include "types/type_pool.h"

namespace stc::glsl {

using namespace stc::types;

inline std::string type_prefix(const TypeDescriptor& td) {
    assert(td.is_scalar() && "trying to get prefix of non-scalar type");

    if (td.is<BoolTD>())
        return "b";

    if (td.is<IntTD>())
        return td.as<IntTD>().is_signed ? "i" : "u";

    if (td.is<FloatTD>())
        return td.as<FloatTD>().width == 32 ? "" : "d";

    return "?";
}

std::string type_str(const TypeDescriptor& td, const TypePool& pool, const SymbolPool& sym_pool);

inline std::string type_str(TypeId type_id, const TypePool& type_pool, const SymbolPool& sym_pool) {
    return type_str(type_pool.get_td(type_id), type_pool, sym_pool);
}

} // namespace stc::glsl
