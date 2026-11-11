#pragma once

#include <fmt/format.h>
#include <iostream>
#include <string>

#include "common/config.h"

namespace stc {

inline std::string indent(size_t level, size_t unit_width, bool use_tabs) {
    return use_tabs ? std::string(level, '\t') : std::string(level * unit_width, ' ');
}

inline std::string indent(size_t level, const TranspilerConfig& config) {
    return indent(level, config.dump_indent, config.use_tabs);
}

void report(std::string_view msg, std::ostream& out = std::cerr);

void error(std::string_view msg, std::ostream& out = std::cerr);
void warning(std::string_view msg, std::ostream& out = std::cerr);
void internal_error(std::string_view msg, std::ostream& out = std::cerr);

inline std::string dump_label(std::string_view label_str) {
    return fmt::format("({}):\n", label_str);
}

void print_header(std::ostream& out, std::string_view header_text,
                  bool extra_trailing_newline = true, bool color_header_text = true,
                  bool empty_line_before = true);

} // namespace stc
