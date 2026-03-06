#pragma once

#include "ir/type_descriptors.h"

namespace stc::ir {

// TODO: use double-interning instead (intern base types and qual types)

enum class Qualifier : uint8_t {
    None      = 0,
    Const     = 1 << 0,
    Volatile  = 1 << 1,
    Restrict  = 1 << 2,
    ReadOnly  = 1 << 3,
    WriteOnly = 1 << 4,
    In        = 1 << 5,
    Out       = 1 << 6,
    // ...
};

inline Qualifier operator|(Qualifier a, Qualifier b) {
    return static_cast<Qualifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool has_any(Qualifier qual, Qualifier mask) {
    return (static_cast<uint8_t>(qual) & static_cast<uint8_t>(mask)) != 0;
}

inline bool has_all(Qualifier qual, Qualifier mask) {
    return (static_cast<uint8_t>(qual) & static_cast<uint8_t>(mask)) == static_cast<uint8_t>(mask);
}

struct Type {
    TypeId base_type;
    Qualifier qualifiers;

    explicit Type(TypeId base_type, Qualifier qualifiers)
        : base_type(base_type), qualifiers(qualifiers) {}

    bool has_qualifiers(Qualifier filter) const;
    std::string to_string() const;

    bool operator==(const Type&) const = default;
};

} // namespace stc::ir