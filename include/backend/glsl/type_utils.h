#pragma once

#include "types/qualifier_pool.h"
#include "types/type_descriptors.h"
#include "types/type_pool.h"

#include <optional>
#include <string>
#include <string_view>

namespace stc::glsl {

using namespace stc::types;

std::optional<std::string_view> type_prefix(const TypeDescriptor& td);

std::optional<std::string> type_str(const TypeDescriptor& td, const TypePool& type_pool,
                                    const SymbolPool& sym_pool);

std::optional<std::string> decl_str(const TypeDescriptor& td, const TypePool& type_pool,
                                    const SymbolPool& sym_pool, SymbolId name);

inline std::optional<std::string> type_str(TypeId type_id, const TypePool& type_pool,
                                           const SymbolPool& sym_pool) {
    return type_str(type_pool.get_td(type_id), type_pool, sym_pool);
}

inline std::optional<std::string> decl_str(TypeId type_id, const TypePool& type_pool,
                                           const SymbolPool& sym_pool, SymbolId name) {
    return decl_str(type_pool.get_td(type_id), type_pool, sym_pool, name);
}

inline std::string_view qual_kind_to_str(QualKind kind) {
    switch (kind) {

#define X(name, ...)                                                                               \
    case (QualKind::tq_##name):                                                                    \
        return #name;

#include "types/qualifier_defs/non_layout.def"

#undef X
#define X(name, ...)                                                                               \
    case (QualKind::lq_##name):                                                                    \
        return #name;

#include "types/qualifier_defs/layout_value.def"
#include "types/qualifier_defs/layout_valueless.def"

        default:
            throw std::logic_error{"unaccounted QualifierKind in qual_kind_to_str"};
    }

#undef X
}

inline uint8_t qual_rank(QualKind kind) {
    // clang-format off
    switch (kind) {
#define X(name, cat, rank)                      \
        case QualKind::tq_##name:               \
            return rank;

#include "types/qualifier_defs/non_layout.def"

#undef X

        default:
            assert(is_layout_qual(kind));
            return 2;
    }
    // clang-format on
}

} // namespace stc::glsl
