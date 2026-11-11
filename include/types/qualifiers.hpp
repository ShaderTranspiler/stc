#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace stc::types {

enum class QualKind : uint8_t {

    FirstNonLQ,
#define X(name, ...) tq_##name,
#define X_FIRST(name, ...) tq_##name = FirstNonLQ,
#define X_LAST(name, ...) tq_##name, LastNonLQ = tq_##name,
#include "types/qualifier_defs/non_layout.def"
#undef X_LAST
#undef X_FIRST
#undef X

    FirstLQ,
    FirstLQNoVal = FirstLQ,
#define X(name, ...) lq_##name,
#define X_FIRST(name, ...) lq_##name = FirstLQNoVal,
#define X_LAST(name, ...) lq_##name, LastLQNoVal = lq_##name,
#include "types/qualifier_defs/layout_valueless.def"
#undef X_LAST
#undef X_FIRST
#undef X

    FirstLQVal,
#define X(name, ...) lq_##name,
#define X_FIRST(name, ...) lq_##name = FirstLQVal,
#define X_LAST(name, ...) lq_##name, LastLQVal = lq_##name,
#include "types/qualifier_defs/layout_value.def"
#undef X_LAST
#undef X_FIRST
#undef X
    LastLQ = LastLQVal

};

constexpr bool is_layout_qual(QualKind kind) {
    return QualKind::FirstLQ <= kind && kind <= QualKind::LastLQ;
}

constexpr bool is_valueless_layout_qual(QualKind kind) {
    return QualKind::FirstLQNoVal <= kind && kind <= QualKind::LastLQNoVal;
}

constexpr bool is_value_layout_qual(QualKind kind) {
    return QualKind::FirstLQVal <= kind && kind <= QualKind::LastLQVal;
}

struct LQPayload {
#define X(name, ...) int32_t lq_##name = 0u;
#include "types/qualifier_defs/layout_value.def"
#undef X

    void set(QualKind kind, int32_t value) {
        if (!is_value_layout_qual(kind))
            throw std::logic_error{
                "LQPayload's set called with non-layout or valueless layout qualifier kind"};

#define X(name, ...)                                                                               \
    if (kind == QualKind::lq_##name) {                                                             \
        this->lq_##name = value;                                                                   \
        return;                                                                                    \
    }
#include "types/qualifier_defs/layout_value.def"
#undef X

        throw std::logic_error{"unexpected qual kind in LQPayload's set"};
    }

    int32_t get_qual_value(QualKind kind) const {
        if (!is_value_layout_qual(kind))
            throw std::logic_error{"get_qual_value called with a qualifier that is not a value "
                                   "carrying layout qualifier"};

        switch (kind) {
#define X(name)                                                                                    \
    case QualKind::lq_##name:                                                                      \
        return lq_##name;

#include "types/qualifier_defs/layout_value.def"

            default:
                throw std::logic_error{"unaccounted qualifier kind in LQPayload's get_qual_value"};
        }
#undef X
    }

    constexpr bool operator==(const LQPayload&) const = default;
};

struct DeclQualifiers {
    std::vector<QualKind> quals;
    LQPayload layout_qual_payloads;

    explicit DeclQualifiers(std::vector<QualKind> quals, LQPayload layout_qual_payloads)
        : quals{std::move(quals)}, layout_qual_payloads{layout_qual_payloads} {}

    DeclQualifiers(const DeclQualifiers&)            = default;
    DeclQualifiers& operator=(const DeclQualifiers&) = default;
    DeclQualifiers(DeclQualifiers&&)                 = default;
    DeclQualifiers& operator=(DeclQualifiers&&)      = default;
    ~DeclQualifiers()                                = default;

    constexpr bool operator==(const DeclQualifiers&) const = default;
};

inline std::optional<QualKind> try_parse_qual(std::string_view str) {
#define X(name, ...)                                                                               \
    if (str == #name)                                                                              \
        return QualKind::tq_##name;

#include "types/qualifier_defs/non_layout.def"
#undef X
#define X(name, ...)                                                                               \
    if (str == #name)                                                                              \
        return QualKind::lq_##name;

#include "types/qualifier_defs/layout_value.def"
#include "types/qualifier_defs/layout_valueless.def"
#undef X

    return std::nullopt;
}

enum class InterfaceStorage : uint8_t { Uniform, In, Out, Buffer };

inline std::string_view iface_storage_str(InterfaceStorage st) {
    switch (st) {
        case InterfaceStorage::In:
            return "in";
        case InterfaceStorage::Out:
            return "out";
        case InterfaceStorage::Uniform:
            return "uniform";
        case InterfaceStorage::Buffer:
            return "buffer";
    }

    throw std::logic_error{"unaccounted storage type in iface_storage_str"};
}

inline QualKind iface_storage_to_qual(InterfaceStorage is) {
    switch (is) {
        case InterfaceStorage::In:
            return QualKind::tq_in;
        case InterfaceStorage::Out:
            return QualKind::tq_out;
        case InterfaceStorage::Uniform:
            return QualKind::tq_uniform;
        case InterfaceStorage::Buffer:
            return QualKind::tq_buffer;
    }

    throw std::logic_error{"unaccounted interface storage type"};
}

} // namespace stc::types
