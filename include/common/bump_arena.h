#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "common/utils.h"

namespace stc {

template <std::unsigned_integral SizeTy>
class BumpArena final {
private:
    static constexpr SizeTy SizeT_max = std::numeric_limits<SizeTy>::max();

    // CLEANUP: wrap these into an ArenaIdx type with overloaded +/-/+=/-= operators
    // helpers to avoid integer promotion issues
    static constexpr SizeTy SizeT_add(SizeTy a, SizeTy b) noexcept {
        return static_cast<SizeTy>(a + b);
    }
    static constexpr SizeTy SizeT_sub(SizeTy a, SizeTy b) noexcept {
        return static_cast<SizeTy>(a - b);
    }
    static constexpr SizeTy SizeT_mul(SizeTy a, SizeTy b) noexcept {
        return static_cast<SizeTy>(a * b);
    }
    static constexpr SizeTy SizeT_div(SizeTy a, SizeTy b) noexcept {
        return static_cast<SizeTy>(a / b);
    }

public:
    static constexpr SizeTy null_id = 0;

    explicit BumpArena(SizeTy initial_slab_capacity = 4096) {
        if (initial_slab_capacity == 0)
            throw std::logic_error{"Slab capacity cannot be zero"};

        slabs.reserve(32);

        make_new_slab(initial_slab_capacity);

        assert(no_nullptrs(slabs_tail, cur_ptr, end_ptr) && !slabs.empty() &&
               !slab_end_offsets.empty() && "broken BumpArena initialization logic");
    }

    ~BumpArena() {
        if (cur_ptr != nullptr)
            run_dtors();
    }

    BumpArena(const BumpArena&)            = delete;
    BumpArena& operator=(const BumpArena&) = delete;

    // default construction is cheap, so move ctor/assignment can be unified through a helper
    BumpArena(BumpArena&& other) noexcept { handle_move(std::move(other)); }

    BumpArena& operator=(BumpArena&& other) noexcept {
        if (this == &other)
            return *this;

        if (cur_ptr != nullptr)
            run_dtors();

        handle_move(std::move(other));

        return *this;
    }

    void reset() {
        assert(cur_ptr != nullptr && "do not reuse moved-from arenas by resetting them");
        assert(!slabs.empty() && "no slabs inside arena");

        run_dtors();

        // this doesn't shrink capacity
        slabs.resize(1);
        slab_end_offsets.resize(1);

        assert(slabs[0] != nullptr);

        last_slab_hint = 0;
        cur_offset     = 1;
        slabs_tail     = slabs[0].get();
        cur_ptr        = slabs[0]->buffer.get();
        end_ptr        = slabs[0]->get_end_ptr();
    }

    bool can_allocate(SizeTy size, SizeTy alignment = 8, std::byte* aligned_ptr = nullptr) const {
        if (aligned_ptr == nullptr)
            aligned_ptr = get_aligned_ptr(cur_ptr, alignment);

        return static_cast<SizeTy>(aligned_ptr - cur_ptr) <=
               SizeT_sub(SizeT_sub(SizeT_max, cur_offset), size);
    }

    template <typename T>
    bool can_allocate(SizeTy n = 1) {
        return can_allocate(SizeT_mul(sizeof(T), n), alignof(T));
    }

    // ! allocations that would not fit into a newly allocated slab are bad_alloc-ed.
    // ! this is intentional, as BumpArena is mostly meant to store small types.
    // ! to work around this, provide an appropriate initial_slab_capacity when creating the arena
    [[nodiscard]] std::pair<SizeTy, void*> allocate(SizeTy size, SizeTy alignment = 8) {
        if (!is_power_of_two(alignment))
            throw std::logic_error{"Alignment value has to be a power of two."};

        if (alignment > alignof(std::max_align_t))
            throw std::logic_error{"Alignments larger than std::max_align_t are not supported"};

        std::byte* aligned_ptr = get_aligned_ptr(cur_ptr, alignment);

        if (aligned_ptr >= end_ptr || size > static_cast<SizeTy>(end_ptr - aligned_ptr)) {
            make_new_slab();
            aligned_ptr = get_aligned_ptr(cur_ptr, alignment);

            if (aligned_ptr >= end_ptr || size > static_cast<SizeTy>(end_ptr - aligned_ptr))
                throw std::bad_alloc{}; // allocation too large for BumpArena
        }

        if (!can_allocate(size, alignment, aligned_ptr))
            throw std::bad_alloc{}; // allocation would overflow offset type

        cur_offset = SizeT_add(cur_offset, static_cast<SizeTy>(aligned_ptr + size - cur_ptr));
        cur_ptr    = aligned_ptr + size;

        if (cur_ptr > end_ptr)
            throw std::bad_alloc{}; // allocation too large for BumpArena

        return {SizeT_sub(cur_offset, size), aligned_ptr};
    }

    template <typename T>
    [[nodiscard]] std::pair<SizeTy, void*> allocate_for(SizeTy n = 1) {
        return allocate(SizeT_mul(sizeof(T), n), alignof(T));
    }

    template <typename T, typename... Args>
    [[nodiscard]] std::pair<SizeTy, T*> emplace(Args&&... args) {
        // allocation and initialization are handled in two passes, so that if T or the dtor would
        // fit into memory and offset, but not together, neither of them get initialized before the
        // bad_alloc is thrown.
        // also, this way, dtor_tail only gets modified if allocations were successful, so the arena
        // remains in a usable state even if the exception is caught and ignored

        void* dtor_mem = nullptr;
        [[maybe_unused]] void (*dtor_call)(void*);

        if constexpr (!std::is_trivially_destructible_v<T>) {
            dtor_mem  = allocate_for<DtorLLNode>().second;
            dtor_call = [](void* obj) { static_cast<T*>(obj)->~T(); };
        }

        auto [offset, mem] = allocate_for<T>();

        T* obj_ptr = new (mem) T{std::forward<Args>(args)...};

        if constexpr (!std::is_trivially_destructible_v<T>) {
            dtor_tail = new (dtor_mem) DtorLLNode{dtor_call, obj_ptr, dtor_tail};
        }

        return {offset, obj_ptr};
    }

    // FEATURE: use fixed sized slabs instead to have slabs[slab_offset].base + offset
    [[nodiscard]] void* get_ptr(SizeTy offset) const {
        if (offset == 0)
            return nullptr;

        const auto& last_accessed = slabs[last_slab_hint];
        assert(last_accessed != nullptr);

        if (last_accessed->contains_offset(offset)) [[likely]]
            return last_accessed->offset_to_ptr(offset);

        auto it = std::upper_bound(slab_end_offsets.begin(), slab_end_offsets.end(), offset);

        if (it == slab_end_offsets.end())
            return nullptr;

        SizeTy idx       = static_cast<SizeTy>(it - slab_end_offsets.begin());
        const auto& slab = slabs[idx];

        assert(slab != nullptr);
        assert(slab->contains_offset(offset) && "wrong slab returned for offset");

        last_slab_hint = idx;

        return slab->offset_to_ptr(offset);
    }

    template <typename T>
    [[nodiscard]] T* get_ptr(SizeTy offset) const {
        return static_cast<T*>(get_ptr(offset));
    }

    // ! only use with pointers that originated from the arena
    // ! assumes ptr points to the beginning of an object in memory
    [[nodiscard]] SizeTy get_offset(const void* ptr) const {
        if (ptr == nullptr)
            return 0U;

        const auto& last_accessed = slabs[last_slab_hint];
        assert(last_accessed != nullptr);

        if (last_accessed->contains_ptr(ptr))
            return last_accessed->ptr_to_offset(ptr);

        // CLEANUP: use binary search
        for (size_t i = 0; i < slabs.size(); i++) {
            const auto& slab = slabs[i];

            if (slab->contains_ptr(ptr)) {
                last_slab_hint = static_cast<SizeTy>(i);
                return slab->ptr_to_offset(ptr);
            }
        }

        return 0U;
    }

    [[nodiscard]] SizeTy get_current_offset() const { return cur_offset; }

private:
    struct Slab {
        std::unique_ptr<std::byte[]> buffer;
        SizeTy capacity;
        SizeTy start_offset;

        explicit Slab(SizeTy capacity, SizeTy start_offset)
            : buffer{std::make_unique_for_overwrite<std::byte[]>(capacity)},
              capacity{capacity},
              start_offset{start_offset} {

            assert(capacity != 0 && "slab capacity cannot be zero");
        }

        Slab(const Slab&)                = delete;
        Slab& operator=(const Slab&)     = delete;
        Slab(Slab&&) noexcept            = default;
        Slab& operator=(Slab&&) noexcept = default;

        // pointer to first byte past buffer
        std::byte* get_end_ptr() const { return buffer.get() + capacity; }

        // offset to first byte past buffer
        SizeTy get_end_offset() const { return SizeT_add(start_offset, capacity); }

        bool contains_offset(SizeTy offset) const {
            return start_offset <= offset && offset < get_end_offset();
        }

        void* offset_to_ptr(SizeTy offset) const {
            assert(contains_offset(offset) && "offset outside slab's buffer");
            return buffer.get() + SizeT_sub(offset, start_offset);
        }

        bool contains_ptr(const void* ptr) const {
            return ptr != nullptr && buffer.get() <= ptr && ptr < get_end_ptr();
        }

        // ! only use with pointers that originated from the arena
        // ! assumes ptr points to the beginning of an object in memory
        SizeTy ptr_to_offset(const void* ptr) const {
            assert(contains_ptr(ptr) && "ptr outside slab's buffer");

            SizeTy internal_offset =
                static_cast<SizeTy>(static_cast<const std::byte*>(ptr) - buffer.get());
            return SizeT_add(start_offset, internal_offset);
        }
    };

    struct DtorLLNode {
        void (*call_dtor)(void*);
        void* obj;
        DtorLLNode* prev;
    };

    std::vector<std::unique_ptr<Slab>> slabs{};
    std::vector<SizeTy> slab_end_offsets{};
    Slab* slabs_tail              = nullptr;
    std::byte* cur_ptr            = nullptr;
    std::byte* end_ptr            = nullptr;
    SizeTy cur_offset             = 1U;
    DtorLLNode* dtor_tail         = nullptr;
    mutable SizeTy last_slab_hint = null_id;

    static uintptr_t get_aligned_size(uintptr_t size, SizeTy alignment) {
        if (size > std::numeric_limits<uintptr_t>::max() - alignment + 1)
            throw std::overflow_error{"The specified pointer alignment would overflow"};

        // below is the same as: (size + alignment - 1U) / alignment * alignment
        // see: https://llvm.org/doxygen/Alignment_8h_source.html#l00144

        // upcast so the bitmask trick works
        uintptr_t alignment_uptr = static_cast<uintptr_t>(alignment);

        return (size + alignment_uptr - 1U) & ~(alignment_uptr - 1U);
    }

    static std::byte* get_aligned_ptr(std::byte* ptr, SizeTy alignment) {
        return reinterpret_cast<std::byte*>(
            get_aligned_size(reinterpret_cast<uintptr_t>(ptr), alignment));
    }

    void run_dtors() {
        while (dtor_tail != nullptr) {
            dtor_tail->call_dtor(dtor_tail->obj);
            dtor_tail = dtor_tail->prev;
        }
    }

    void make_new_slab(SizeTy capacity_fallback = 0) {
        SizeTy new_capacity;
        if (!slabs.empty()) {
            assert(slabs[slabs.size() - 1] != nullptr && "Slab pointer is nullptr");
            SizeTy old_capacity = slabs[slabs.size() - 1]->capacity;
            new_capacity =
                old_capacity < SizeT_div(SizeT_max, 2) ? SizeT_mul(old_capacity, 2U) : SizeT_max;
        } else {
            assert(capacity_fallback != 0 &&
                   "creating first slab through make_new_slab without a fallback value");
            new_capacity = capacity_fallback;
        }

        SizeTy start_offset = !slabs.empty() ? slabs_tail->get_end_offset() : 1;

        // prevent unnecessary offset overflows with smaller idx types
        // by limiting slab sizes based on max idx from start_offset
        new_capacity = std::min(new_capacity, SizeT_sub(SizeT_max, start_offset));

        if (new_capacity == 0)
            throw std::overflow_error{"BumpArena index type exhausted"};

        slabs.push_back(std::make_unique<Slab>(new_capacity, start_offset));
        slab_end_offsets.push_back(SizeT_add(start_offset, new_capacity));

        slabs_tail = slabs.back().get();
        cur_ptr    = slabs_tail->buffer.get();
        cur_offset = slabs_tail->start_offset;
        end_ptr    = slabs_tail->get_end_ptr();
    }

    void handle_move(BumpArena&& other) noexcept {
        slabs            = std::move(other.slabs);
        slab_end_offsets = std::move(other.slab_end_offsets);
        slabs_tail       = other.slabs_tail;
        cur_ptr          = other.cur_ptr;
        end_ptr          = other.end_ptr;
        cur_offset       = other.cur_offset;
        dtor_tail        = other.dtor_tail;
        last_slab_hint   = other.last_slab_hint;

        other.cur_ptr = other.end_ptr = nullptr;
        other.dtor_tail               = nullptr;
        other.slabs_tail              = nullptr;
        other.cur_offset              = null_id;
        other.last_slab_hint          = null_id;
    }
};

using BumpArena16 = BumpArena<uint16_t>;
using BumpArena32 = BumpArena<uint32_t>;
using BumpArena64 = BumpArena<uint64_t>;

template <typename T>
struct is_bump_arena : std::false_type {};

template <typename SizeTy>
struct is_bump_arena<BumpArena<SizeTy>> : std::true_type {};

template <typename T>
concept CBumpArena = is_bump_arena<T>::value;

}; // namespace stc
