#pragma once

#include <optional>
#include <string_view>

#include "common/bump_arena.h"
#include "common/id_types.h"
#include "common/utils.h"

namespace stc {

class SymbolPool {
    using ArenaTy = BumpArena<SymbolId::id_type>;

public:
    explicit SymbolPool(SymbolId::id_type initial_capacity_bytes)
        : arena{initial_capacity_bytes}, pool{} {}

    SymbolPool(const SymbolPool&)            = delete;
    SymbolPool& operator=(const SymbolPool&) = delete;
    SymbolPool(SymbolPool&&) noexcept        = default;
    SymbolPool& operator=(SymbolPool&&)      = default;

    ~SymbolPool() = default;

    ErasedArenaView get_erased_arena_view() const {
        return {
            .name    = "Symbol Pool",
            .arena   = &arena,
            .size_id = ErasedArenaView::get_size_id<SymbolId::id_type>(),
        };
    }

    SymbolId get_id(std::string_view symbol_value);
    std::string_view get_symbol(SymbolId id) const;
    bool contains(SymbolId id) const;
    bool contains(std::string_view symbol) const;
    std::optional<std::string_view> get_symbol_maybe(SymbolId id) const;

private:
    ArenaTy arena;

    std::unordered_map<std::string_view, SymbolId> pool;
};

} // namespace stc
