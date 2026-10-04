#pragma once

#include <charconv>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

namespace stc::cli {

void print_help();

void print_version_info();

std::filesystem::path tail_from_cwd(const std::filesystem::path& p) {
    namespace fs = std::filesystem;

    fs::path abs = fs::absolute(p);
    fs::path cwd = fs::current_path();

    fs::path rel = abs.lexically_relative(cwd);

    if (!rel.empty())
        return rel;

    return abs;
}

template <typename T>
requires requires (const char* c, T t) { std::from_chars(c, c, t); }
std::optional<T> try_parse_num(const std::string& str) {
    T value = 0;

    const char* str_end  = str.data() + str.size();
    auto [tail_ptr, err] = std::from_chars(str.data(), str_end, value);

    // ptr check ensures the entire string matches and not just the prefix
    // e.g. 12a is rejected instead of getting parsed as 12 + tail
    if (err == std::errc() && tail_ptr == str_end)
        return value;

    return std::nullopt;
}

} // namespace stc::cli
