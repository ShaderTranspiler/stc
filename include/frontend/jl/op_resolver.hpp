#pragma once

#include <vector>

#include "ast/symbol_pool.hpp"
#include "common/target_info.hpp"

namespace stc::jl {

struct OpMapping {
    static_assert(CEnumOf<TargetOpKind, uint8_t>);
    static constexpr TargetOpKind INVALID_OP = static_cast<TargetOpKind>(255);

    TargetOpKind unop  = INVALID_OP;
    TargetOpKind binop = INVALID_OP;
};

class OpResolver {
private:
    // stored in sorted list (N too small for hashmap)
    // CLEANUP: profile vector/unordered_map perf (here and in ctor_resolver)
    std::vector<std::pair<SymbolId, OpMapping>> op_map{};

public:
    explicit OpResolver(SymbolPool& sym_pool);

    // returns OpMapping::INVALID_ID on failure
    TargetOpKind get_target_op(SymbolId op_sym, bool is_binop) const;
};

} // namespace stc::jl
