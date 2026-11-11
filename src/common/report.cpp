#include <iostream>

#include "common/report.h"
#include "common/term_utils.h"

namespace stc {

void report(std::string_view msg, std::ostream& out) {
    out << msg << '\n';
}

void error(std::string_view msg, std::ostream& out) {
    out << colored("[Error] ", ansi_codes::red);
    report(msg, out);
}

void warning(std::string_view msg, std::ostream& out) {
    out << colored("[Warning] ", ansi_codes::yellow);
    report(msg, out);
}

void internal_error(std::string_view msg, std::ostream& out) {
    out << colored("[Internal Error] ", ansi_codes::red);
    report(msg, out);
}

void print_header(std::ostream& out, std::string_view header_text, bool extra_trailing_newline,
                  bool color_header_text, bool empty_line_before) {
    static constexpr size_t header_padding_amount = 3;

    std::string separator(header_text.length() + 2 * header_padding_amount, '=');
    std::string padding(header_padding_amount, ' ');

    if (empty_line_before)
        out << '\n';

    out << separator << '\n';
    out << padding;

    if (color_header_text) // this can't be a ternary since colored returns stc::ColoredText
        out << colored(header_text, ansi_codes::yellow);
    else
        out << header_text;

    out << padding << '\n';
    out << separator << '\n';

    if (extra_trailing_newline)
        out << '\n';

    out.flush();
}

} // namespace stc
