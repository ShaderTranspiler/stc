#pragma once

#include <chrono>
#include <ostream>
#include <string_view>

#include "common/bump_arena.h"

namespace stc::api::detail {

template <bool Enabled>
struct BenchmarkTracker {
    void start() {}
    void init_start() {}
    void init_end() {}
    void parser_start() {}
    void parser_end() {}
    void sema_start() {}
    void sema_end() {}
    void lowering_start() {}
    void lowering_end() {}
    void code_gen_start() {}
    void code_gen_end() {}
    void end() {}

    void print([[maybe_unused]] std::ostream& out) const {}
};

template <>
struct BenchmarkTracker<true> {
    using clock = std::chrono::steady_clock;

private:
    std::chrono::time_point<clock> tp_start, tp_init_start, tp_init_end, tp_parser_start,
        tp_parser_end, tp_sema_start, tp_sema_end, tp_lowering_start, tp_lowering_end,
        tp_code_gen_start, tp_code_gen_end, tp_end;

public:
    void start() { tp_start = clock::now(); }
    void init_start() { tp_init_start = clock::now(); }
    void init_end() { tp_init_end = clock::now(); }
    void parser_start() { tp_parser_start = clock::now(); }
    void parser_end() { tp_parser_end = clock::now(); }
    void sema_start() { tp_sema_start = clock::now(); }
    void sema_end() { tp_sema_end = clock::now(); }
    void lowering_start() { tp_lowering_start = clock::now(); }
    void lowering_end() { tp_lowering_end = clock::now(); }
    void code_gen_start() { tp_code_gen_start = clock::now(); }
    void code_gen_end() { tp_code_gen_end = clock::now(); }
    void end() { tp_end = clock::now(); }

    void print(std::ostream& out) const;
};

static_assert(sizeof(BenchmarkTracker<false>) == 1U);

template <bool Enabled>
struct MemoryTracker {
    void register_arena([[maybe_unused]] ErasedArenaView erased_arena) const {}
    void unregister_arena([[maybe_unused]] const void* arena_ptr) const {}

    void print([[maybe_unused]] std::string_view dump_name,
               [[maybe_unused]] std::ostream& out) const {}
};

template <>
struct MemoryTracker<true> {
private:
    static constexpr size_t expected_num_of_arenas = 8;
    std::vector<ErasedArenaView> arenas{};

public:
    MemoryTracker() { arenas.reserve(expected_num_of_arenas); }

    void register_arena(ErasedArenaView arena_view) { arenas.emplace_back(arena_view); }
    void unregister_arena(const void* arena_ptr);

    void print(std::string_view dump_name, std::ostream& out) const;
};

static_assert(sizeof(MemoryTracker<false>) == 1U);

} // namespace stc::api::detail
