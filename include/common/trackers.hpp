#pragma once

#include <chrono>
#include <ostream>
#include <string_view>

#include "common/bump_arena.hpp"

namespace stc {

struct TimingTracker {
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration  = Clock::duration;

private:
    struct Timing {
        std::string_view name;
        TimePoint start, end;
        bool include_in_sum;
    };

    // init, parser, sema, lowering, code gen
    static constexpr size_t expected_num_of_timings = 5;

    // assumptions whenever print() is called:
    // timings.front().start == transpilation begin time
    // timings.end().end == transpilation end time
    std::vector<Timing> timings;

    bool enabled;

public:
    explicit TimingTracker(bool enabled)
        : enabled{enabled} {
        if (enabled)
            timings.reserve(expected_num_of_timings);
    }

    // only exclusive, sequential timing is allowed, that is:
    // after a call to start_timing(s, tp1), the next method call should be end_timing(tp2),
    // indicating the end of recording timing for 's', where tp1 <= tp2
    // the name can be reused for different timings, but it will be treated as a new, independent
    // timing section
    void start_timing(std::string_view name, bool include_in_sum = true,
                      TimePoint start_time = Clock::now());
    void end_timing(TimePoint end_time = Clock::now()) noexcept;

    void print(std::ostream& out) const;

    bool is_enabled() const { return enabled; }

    // RAII-ish timer
    // we still assume timings cannot overlap, this is just  useful for automatically ending timings
    // on throwing paths and not leaving a tracker entry open
    struct Timer {
    private:
        TimingTracker& tracker;
        bool is_running_ = false;

    public:
        explicit Timer(TimingTracker& tracker)
            : tracker{tracker} {}

        ~Timer() {
            if (is_running_)
                tracker.end_timing();
        }

        Timer(const Timer&) = delete;
        Timer(Timer&& other) noexcept
            : tracker{other.tracker}, is_running_{other.is_running_} {
            other.is_running_ = false;
        };

        Timer& operator=(const Timer&)  = delete;
        Timer& operator=(Timer&& other) = delete;

        bool is_running() const { return is_running_; }

        void start(std::string_view name, bool include_in_sum = true,
                   TimePoint start_time = Clock::now()) {
            // same debug vs release reasoning as in end()
            assert(!is_running_ && "trying to start timer while it is already running");
            if (is_running_)
                end();
            assert(!is_running_);

            tracker.start_timing(name, include_in_sum, start_time);
            is_running_ = true;
        }

        void end(TimePoint end_time = Clock::now()) noexcept {
            // this indicates some start/end mismatch at the caller site
            // we can just ignore the request in release builds, but it's nice to have some checking
            // in debug builds
            assert(is_running_ && "trying to end timer that is not currently running");
            if (!is_running_)
                return;

            tracker.end_timing(end_time);
            is_running_ = false;
        }
    };

    Timer create_timer() { return Timer(*this); }
};

struct MemoryTracker {
private:
    static constexpr size_t expected_num_of_arenas = 8;
    std::vector<ErasedArenaView> arenas{};
    bool enabled;

public:
    explicit MemoryTracker(bool enabled)
        : enabled{enabled} {
        if (enabled)
            arenas.reserve(expected_num_of_arenas);
    }

    void register_arena(ErasedArenaView arena_view) {
        if (enabled)
            arenas.emplace_back(arena_view);
    }

    void unregister_arena(const void* arena_ptr, bool fail_if_not_tracked = true);

    void print(std::string_view dump_name, std::ostream& out) const;

    bool is_enabled() const { return enabled; }
};

} // namespace stc
