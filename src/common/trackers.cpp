#include "common/trackers.h"

#include <algorithm>
#include <cassert>
#include <fmt/format.h>
#include <numeric>
#include <stdexcept>

#include "common/report.h"
#include "common/term_utils.h"

namespace {

using namespace stc;

std::string format_duration(std::chrono::nanoseconds dur) {
    using namespace std::chrono;

    duration<double> exact_sec            = dur;
    duration<double, std::milli> exact_ms = dur;
    duration<double, std::micro> exact_us = dur;

    if (duration_cast<seconds>(dur).count() > 0)
        return fmt::format("{:.3f} sec", exact_sec.count());

    if (duration_cast<milliseconds>(dur).count() > 0)
        return fmt::format("{:.3f} ms", exact_ms.count());

    return fmt::format("{:.0f} us", exact_us.count());
}

template <std::unsigned_integral SizeTy>
std::string format_bytes(SizeTy bytes) {
    if (bytes < 1024)
        return fmt::format("{} bytes", bytes);

    auto value = static_cast<double>(bytes);
    // clang-format off
    if ((value /= 1024.0) < 1024) return fmt::format("{:.2f} KiB", value);
    if ((value /= 1024.0) < 1024) return fmt::format("{:.2f} MiB", value);
    if ((value /= 1024.0) < 1024) return fmt::format("{:.2f} GiB", value);
    // clang-format on

    // if we hit 1 TiB+ memory usage, we have much bigger problems than formatting
    return fmt::format("{:.2f} TiB", value);
}

template <std::integral SizeTy>
std::string format_percentage(SizeTy current, SizeTy overall) {
    if (overall == 0U)
        return "0.00%";

    double percentage = static_cast<double>(current) / static_cast<double>(overall);
    percentage *= 100.0;
    return fmt::format("{:.2f}%", percentage);
}

template <std::unsigned_integral SizeTy>
void print_arena(std::string_view name, const stc::BumpArena<SizeTy>& arena, std::ostream& out) {
    typename stc::BumpArena<SizeTy>::ArenaMemoryUsage arena_usage = arena.memory_usage();

    stc::print_header(out, name, false);

    auto print_stat = [&out](std::string_view stat_name, SizeTy current,
                             std::optional<SizeTy> overall = std::nullopt) {
        out << stat_name << fmt::format(": {}", format_bytes(current));
        if (overall.has_value())
            out << fmt::format(" ({})", format_percentage(current, *overall));
        out << '\n';
    };

    // total arena usage is a sum of per-slab usage
    // printing slab data under 2 slabs is meaningless
    bool print_total_only = arena_usage.per_slab_usage.size() < 2;

    if (!print_total_only)
        stc::print_header(out, "Total", false);

    out << "Slab count: " << arena_usage.per_slab_usage.size() << '\n';
    print_stat("Allocated", arena_usage.allocated_bytes);
    print_stat("Used", arena_usage.used_bytes, arena_usage.allocated_bytes);
    assert(arena_usage.effective_bytes <= arena_usage.used_bytes);
    print_stat("Effective", arena_usage.effective_bytes, arena_usage.used_bytes);

    if (print_total_only)
        return;

    for (size_t i = 0; i < arena_usage.per_slab_usage.size(); i++) {
        const auto& slab_usage = arena_usage.per_slab_usage[i];

        stc::print_header(out, fmt::format("Slab #{}", i + 1), false);
        print_stat("Allocated", slab_usage.allocated_bytes);
        print_stat("Used", slab_usage.used_bytes, slab_usage.allocated_bytes);
        assert(slab_usage.effective_bytes <= slab_usage.used_bytes);
        print_stat("Effective", slab_usage.effective_bytes, slab_usage.used_bytes);
    }
}

void dispatch_print_arena(const stc::ErasedArenaView& arena_view, std::ostream& out) {
    using ArenaSizeID = stc::ErasedArenaView::ArenaSizeID;
    using enum ArenaSizeID;

    if (arena_view.arena == nullptr)
        throw std::logic_error{"nullptr in erased arena view!"};

    auto dispatch = [&arena_view, &out](const auto& arena) {
        print_arena(arena_view.name, arena, out);
    };

    ArenaSizeID size_id = arena_view.size_id;
    // clang-format off
    switch (size_id) {
        case U8:  return dispatch(*static_cast<const stc::BumpArena<uint8_t >*>(arena_view.arena));
        case U16: return dispatch(*static_cast<const stc::BumpArena<uint16_t>*>(arena_view.arena));
        case U32: return dispatch(*static_cast<const stc::BumpArena<uint32_t>*>(arena_view.arena));
        case U64: return dispatch(*static_cast<const stc::BumpArena<uint64_t>*>(arena_view.arena));
        default:  throw std::logic_error{"Invalid size ID in erased arena view!"};
    }
    // clang-format on
}

} // namespace

namespace stc {

void TimingTracker::start_timing(std::string_view name, bool include_in_sum, TimePoint start_time) {
    if (enabled)
        timings.emplace_back(name, start_time, TimePoint::min(), include_in_sum);
}

void TimingTracker::end_timing(TimePoint end_time) noexcept {
    if (enabled) {
        // same debug vs release logic as in Timer::end()
        assert(!timings.empty() && "trying to end a timing that wasn't started");
        if (!timings.empty())
            timings.back().end = end_time;
    }
}

void TimingTracker::print(std::ostream& out) const {
    if (!enabled)
        return;

    // timing names included in sum get 4 extra characters: " (*)"
    auto real_name_length = [](const Timing& timing) {
        return timing.include_in_sum ? timing.name.length() + 4 : timing.name.length();
    };

    size_t max_name_length = std::transform_reduce(
        timings.begin(), timings.end(), size_t{0}, [](size_t a, size_t b) { return a > b ? a : b; },
        real_name_length);

    Duration timings_sum = std::accumulate(timings.begin(), timings.end(), Duration::zero(),
                                           [](auto acc, const auto& timing) {
                                               if (!timing.include_in_sum)
                                                   return acc;

                                               return acc + (timing.end - timing.start);
                                           });

    print_header(out, "Timings");

    for (const Timing& timing : timings) {
        assert(timing.end != TimePoint::min());
        assert(timing.start <= timing.end);

        Duration timing_duration = timing.end - timing.start;

        out << colored(timing.name, ansi_codes::yellow);
        if (timing.include_in_sum)
            out << colored(" (*)", ansi_codes::yellow);
        assert(max_name_length >= real_name_length(timing));
        std::string padding(max_name_length - real_name_length(timing), ' ');
        out << padding;
        out << " finished in ";
        out << colored(format_duration(timing_duration), ansi_codes::yellow);
        if (timing.include_in_sum) {
            out << colored(fmt::format(" ({})", format_percentage(timing_duration.count(),
                                                                  timings_sum.count())),
                           ansi_codes::yellow);
        }
        out << '\n';
    }

    out << "\nEntire transpilation finished in "
        << colored(format_duration(timings.back().end - timings.front().start), ansi_codes::yellow);
    out << ", sum of " << colored("(*)", ansi_codes::yellow) << " is "
        << colored(format_duration(timings_sum), ansi_codes::yellow) << '\n';

    out.flush();
}

void MemoryTracker::unregister_arena(const void* arena_ptr, bool fail_if_not_tracked) {
    if (!enabled)
        return;

    auto it = std::find_if(arenas.begin(), arenas.end(),
                           [=](const auto& arena_view) { return arena_view.arena == arena_ptr; });

    if (it == arenas.end()) {
        if (!fail_if_not_tracked)
            return;

        throw std::logic_error{"Trying to unregister arena not under memory tracking"};
    }

    arenas.erase(it);
}

void MemoryTracker::print(std::string_view dump_name, std::ostream& out) const {
    if (!enabled)
        return;

    print_header(out, fmt::format("Arena Memory Usage ({})", dump_name), false);
    for (const auto& arena_view : arenas)
        dispatch_print_arena(arena_view, out);
}

} // namespace stc
