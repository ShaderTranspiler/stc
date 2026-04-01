#pragma once

#include <utility>

namespace stc {

template <typename To, typename From>
concept CDynCastable = std::derived_from<To, From> && requires (From from) {
    typename From::kind_type;
    { from.kind() } -> std::same_as<typename From::kind_type>;
    { To::same_node_kind(std::declval<typename From::kind_type>()) } -> std::same_as<bool>;
};

template <typename To, typename From>
requires CDynCastable<To, From>
bool isa(From* ptr) {
    return ptr != nullptr && To::same_node_kind(ptr->kind());
}

template <typename... To, typename From>
requires (sizeof...(To) > 1) && (CDynCastable<To, From> && ...) && (!std::same_as<To, From> && ...)
bool isa(From* ptr) {
    return (isa<To, From>(ptr) || ...);
}

template <typename To, typename From>
requires CDynCastable<To, From>
To* dyn_cast(From* ptr) {
    if (ptr == nullptr)
        return nullptr;

    if (To::same_node_kind(ptr->kind()))
        return static_cast<To*>(ptr);

    return nullptr;
}

} // namespace stc