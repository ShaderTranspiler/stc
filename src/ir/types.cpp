#include "ir/types.h"

namespace stc::ir {

bool Type::has_qualifiers(Qualifier filter) const {
    return has_any(qualifiers, filter);
}

std::string Type::to_string() const {
    return "TypeId: " + std::to_string(base_type);
}

} // namespace stc::ir