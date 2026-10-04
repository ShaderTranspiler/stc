#include "cli_utils.h"

#include <common/term_utils.h>
#include <meta.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <span>
#include <string_view>

namespace {

template <size_t N>
consteval std::array<char, N> n_chars_of(char sep) {
    std::array<char, N> arr{};

    for (size_t i = 0; i < N; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        arr[i] = sep;
    }

    return arr;
}

} // namespace

void stc::cli::print_version_info() {
    static constexpr auto val_col = ansi_codes::yellow;

    static constexpr std::string_view title = "Shader Transpiler Core (STC)";

    static constexpr uint8_t padding_width = 5;
    static constexpr auto padding_buf      = n_chars_of<padding_width>(' ');
    static constexpr auto padding = std::string_view{padding_buf.data(), padding_buf.size()};

    static constexpr size_t header_width = 2U * static_cast<size_t>(padding_width) + title.size();

    static constexpr auto sep_line_buf = n_chars_of<header_width>('-');
    static constexpr auto sep_line     = std::string_view{sep_line_buf.data(), sep_line_buf.size()};

    std::cout << sep_line << '\n';
    std::cout << padding << title << padding << '\n';
    std::cout << sep_line << "\n\n";

    // clang-format off
    std::cout << "version:        " << stc::colored(stc::meta::version, val_col) << '\n';
    std::cout << "target:         " << stc::colored(fmt::format("{}-{}", stc::meta::system_arch, stc::meta::system_name), val_col) << '\n';
    std::cout << "compiler:       " << stc::colored(fmt::format("{} {}", stc::meta::compiler_id, stc::meta::compiler_ver), val_col) << '\n';
    std::cout << "julia compat:   " << stc::colored(stc::meta::julia_ver, val_col) << '\n';
    std::cout << "build origin:   " << stc::colored(stc::meta::build_origin, val_col) << '\n';
    std::cout << "build type:     " << stc::colored(stc::meta::build_type, val_col) << '\n';
    std::cout << "build commit:   " << stc::colored(stc::meta::build_commit, val_col) << '\n';
    std::cout << "build time:     " << stc::colored(stc::meta::build_date, val_col) << '\n';
    // clang-format on
}

namespace {

struct HelpEntry {
    std::string_view name;
    std::string_view description;
};

struct HelpCategory {
    std::string_view name;
    std::span<const HelpEntry> entries;
};

// clang-format off
constexpr std::array general_entries{
    HelpEntry{"-h, --help",        "show this help message and exit"},
    HelpEntry{"-v, --version",     "show version information and exit"},
    HelpEntry{"-o <path>",         "set output file path (default: only print to stdout)"},
    HelpEntry{"--no-out",          "do not output the generated code to stdout or disk"},
    HelpEntry{"--gl-version <N>",  "set OpenGL version for #version directive (default: \"460\")"},
    HelpEntry{"--it <N>",          "run transpilation N times (for benchmarking)"},
    HelpEntry{"--no-benchmark",    "disable the measuring and printing of transpilation time benchmarks"},
    HelpEntry{"--pre-eval <path>", "evaluate the contents of <path> in the Julia context before transpilation"}
};

constexpr std::array transpilation_behavior_entries{
    HelpEntry{"--conv-fail-reason",         "print reason for conversion/casting failures"},
    HelpEntry{"--no-coerce-i32",            "disable automatic Int64 -> Int32 literal type coercion"},
    HelpEntry{"--no-coerce-f32",            "disable automatic Float64 -> Float32 literal type coercion"},
    HelpEntry{"--no-coercion",              "disable all literal type coercion"},
    HelpEntry{"--fwd-fns",                  "enable blind forwarding for non-backend-resolvable function calls (may lead to invalid code)"},
    HelpEntry{"--no-uniform-capture",       "don't capture Julia-resolvable globals as uniforms in the output"}
};

constexpr std::array debugging_entries{
    HelpEntry{"--dump-parsed",    "print the parsed version of the Julia AST"},
    HelpEntry{"--dump-sema",      "print the AST after semantic analysis"},
    HelpEntry{"--dump-lowered",   "print the lowered (SIR) representation"},
    HelpEntry{"--dump-scopes",    "dump scope tree info during semantic analysis"},
    HelpEntry{"--track-bindings", "step-by-step reasoning for symbol binding resolution"}
};

constexpr std::array formatting_entries{
    HelpEntry{"--tabs",          "use tabs for indentation"},
    HelpEntry{"--spaces",        "use spaces for indentation (default)"},
    HelpEntry{"--cg-indent <n>", "set indentation width (default: 4)"}
};

constexpr std::array errors_and_warnings_entries{
    HelpEntry{"--err-dump none",    "disable AST dumping on errors (default)"},
    HelpEntry{"--err-dump partial", "dump the AST subtree that caused the given error"},
    HelpEntry{"--err-dump verbose", "dump the full AST at every error reported"},
    HelpEntry{"-Wjl-query",         "warn when the Julia runtime was queried for function call resolution"}
};

constexpr std::array help_categories{
    HelpCategory{"general", general_entries},
    HelpCategory{"transpilation behavior", transpilation_behavior_entries},
    HelpCategory{"debugging", debugging_entries},
    HelpCategory{"formatting", formatting_entries},
    HelpCategory{"errors and warnings", errors_and_warnings_entries}
};
// clang-format on

} // namespace

void stc::cli::print_help() {
    static constexpr auto title_col = ansi_codes::cyan;
    static constexpr auto flag_col  = ansi_codes::yellow;

    std::cout << stc::colored("usage:", title_col) << " stc <input file> [OPTIONS]\n\n";

    for (size_t i = 0; i < help_categories.size(); i++) {
        const auto& category = help_categories[i];

        assert(!category.entries.empty() && "help category without entries");
        size_t max_width = category.entries[0].name.size();
        for (size_t j = 1; j < category.entries.size(); j++)
            max_width = std::max(category.entries[j].name.size(), max_width);

        std::cout << stc::colored(fmt::format("{}:\n", category.name), title_col);

        for (const auto& entry : category.entries) {
            std::cout << "  " << stc::colored(entry.name, flag_col)
                      << std::string(max_width + 2 - entry.name.size(), ' ') << entry.description
                      << '\n';
        }

        if (i != help_categories.size() - 1)
            std::cout << '\n';
    }
}

std::filesystem::path stc::cli::tail_from_cwd(const std::filesystem::path& p) {
    namespace fs = std::filesystem;

    fs::path abs = fs::absolute(p);
    fs::path cwd = fs::current_path();

    fs::path rel = abs.lexically_relative(cwd);

    if (!rel.empty())
        return rel;

    return abs;
}
