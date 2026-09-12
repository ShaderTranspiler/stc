#pragma once

#include "common/bump_arena.h"

namespace stc {

// NOTE:
// although this was designed to be usable both on its own, and for STL containers, it has not been
// properly tested with them yet
template <std::unsigned_integral SizeTy, typename T>
class BumpArenaAllocator final {
public:
    using value_type = T;

    explicit BumpArenaAllocator(BumpArena<SizeTy>* arena) noexcept
        : arena{arena} {}

    template <typename U>
    BumpArenaAllocator(const BumpArenaAllocator<SizeTy, U>& other) noexcept
        : arena(other.arena) {}

    template <typename U>
    friend class ArenaAllocator;

    [[nodiscard]] T* allocate(size_t n) {
        return static_cast<T*>((arena->template allocate_for<T>(n)).second);
    }

    // no-op because of arena design, memory is deallocated together
    void deallocate([[maybe_unused]] T* start, [[maybe_unused]] size_t size) noexcept {}

    // NOTE: this is an extra helper, potentially useful for non-STL DSA stuff
    template <typename... Args>
    [[nodiscard]] std::pair<SizeTy, T*> emplace(Args&&... args) {
        return arena->template emplace<T>(std::forward<Args>(args)...);
    }

    bool can_allocate(SizeTy n = 1) { return arena->template can_allocate<T>(n); }

    bool operator==(const BumpArenaAllocator& other) { return arena == other.arena; }
    bool operator!=(const BumpArenaAllocator& other) { return !(*this == other); }

private:
    BumpArena<SizeTy>* arena;
};

template <typename T>
using BumpArenaAllocator16 = BumpArenaAllocator<uint16_t, T>;

template <typename T>
using BumpArenaAllocator32 = BumpArenaAllocator<uint32_t, T>;

template <typename T>
using BumpArenaAllocator64 = BumpArenaAllocator<uint64_t, T>;

} // namespace stc
