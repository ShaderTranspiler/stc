#pragma once

#include <array>
#include <cassert>
#include <chrono>
#include <fmt/format.h>
#include <numeric>
#include <ostream>
#include <string_view>

#include "common/report.h"
#include "common/term_utils.h"

namespace stc::api::detail {

inline std::string format_bm_duration(std::chrono::nanoseconds dur) {
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

    std::chrono::time_point<clock> tp_start, tp_init_start, tp_init_end, tp_parser_start,
        tp_parser_end, tp_sema_start, tp_sema_end, tp_lowering_start, tp_lowering_end,
        tp_code_gen_start, tp_code_gen_end, tp_end;

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

    void print(std::ostream& out) const {
        using namespace std::string_view_literals;

        static constexpr std::array phase_display_names = {
            "Transpiler init"sv, "Expr parser"sv,     "Julia sema"sv,
            "AST lowering"sv,    "Code generation"sv,
        };

        static constexpr size_t max_name_length = std::transform_reduce(
            phase_display_names.begin(), phase_display_names.end(), size_t{0},
            [](size_t a, size_t b) { return a > b ? a : b; },
            [](std::string_view el) { return el.length(); });

        std::array phase_times = {
            tp_init_end - tp_init_start,         tp_parser_end - tp_parser_start,
            tp_sema_end - tp_sema_start,         tp_lowering_end - tp_lowering_start,
            tp_code_gen_end - tp_code_gen_start,
        };

        print_header(out, "Benchmark");

        static_assert(phase_display_names.size() == phase_times.size());
        for (size_t i = 0; i < phase_display_names.size(); i++) {
            out << colored(phase_display_names[i], ansi_codes::yellow);
            assert(max_name_length >= phase_display_names[i].length());
            std::string padding(max_name_length - phase_display_names[i].length(), ' ');
            out << padding;
            out << " finished in ";
            out << colored(format_bm_duration(phase_times[i]), ansi_codes::yellow);
            out << '\n';
        }

        out << "\nEntire transpilation finished in "
            << colored(format_bm_duration(tp_end - tp_start), ansi_codes::yellow) << '\n';

        out.flush();
    }
};

static_assert(sizeof(BenchmarkTracker<false>) == 1U);

} // namespace stc::api::detail
