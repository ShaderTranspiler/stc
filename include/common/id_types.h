#pragma once

#include <cstdint>

#include "common/utils.h"

namespace stc {

#define STC_DEFINE_ID_TYPE(name, underlying_type)                                                  \
    struct name : public ::stc::StrongId<underlying_type> {                                        \
        using StrongId::StrongId;                                                                  \
        static constexpr name null_id() { return name(0U); }                                       \
        [[nodiscard]] constexpr bool is_null() const { return *this == null_id(); }                \
    }

#define STC_DEFINE_SPLIT_U32_ID_TYPE(name)                                                         \
    struct name : public SplitU32Id {                                                              \
        using SplitU32Id::SplitU32Id;                                                              \
        static constexpr name null_id() { return name{0U, 0U}; }                                   \
        bool is_null() const { return *this == null_id(); }                                        \
    }

STC_DEFINE_ID_TYPE(SrcLocationId, uint32_t); // see src_info.h
STC_DEFINE_ID_TYPE(SymbolId, uint32_t);      // see symbol_pool.h

namespace types {

STC_DEFINE_ID_TYPE(TypeId, uint16_t); // see type_descriptors.h and type_pool.h
STC_DEFINE_ID_TYPE(QualId, uint16_t); // see qualifier_pool.h

} // namespace types

namespace jl {

STC_DEFINE_SPLIT_U32_ID_TYPE(NodeId);

} // namespace jl

namespace sir {

STC_DEFINE_SPLIT_U32_ID_TYPE(NodeId);

} // namespace sir

} // namespace stc

#undef STC_DEFINE_SPLIT_U32_ID_TYPE
#undef STC_DEFINE_ID_TYPE
