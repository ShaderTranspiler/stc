#include <iostream>

#include "common/utils.h"

namespace stc {

void report(std::string_view msg, std::string_view prefix, std::ostream& out) {
    out << prefix << msg << '\n';
}

void error(std::string_view msg, std::ostream& out) {
    report(msg, "Error: ", out);
}

void warning(std::string_view msg, std::ostream& out) {
    report(msg, "Warning: ", out);
}

void internal_error(std::string_view msg, std::ostream& out) {
    report(msg, "Internal transpiler error: ", out);
}

} // namespace stc