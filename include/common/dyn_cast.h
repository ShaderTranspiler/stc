#pragma once

#include <memory>

namespace stc {

template <typename To, typename From>
concept CDynCastable = std::derived_from<To, From> && requires (From* ptr) {
    { To::same_node_t(ptr) } -> std::same_as<bool>;
};

template <typename To, typename From>
requires CDynCastable<To, From>
To* dyn_cast(From* ptr) {
    if (ptr == nullptr)
        return nullptr;

    if (To::same_node_t(ptr)) {
        return static_cast<To*>(ptr);
    }

    return nullptr;
}

template <typename To, typename From>
requires CDynCastable<To, From>
std::unique_ptr<To> dyn_unique_cast(std::unique_ptr<From>&& ptr) {
    if (ptr == nullptr)
        return nullptr;

    if (auto* cast_ptr = dyn_cast<To, From>(ptr.get())) {
        std::ignore = ptr.release();
        return std::unique_ptr<To>{cast_ptr};
    }

    return nullptr;
}

} // namespace stc