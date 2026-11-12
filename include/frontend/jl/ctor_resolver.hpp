#pragma once

#include "ast/symbol_pool.hpp"
#include <vector>

namespace stc::jl {

class CtorResolver {
    // vector instead of map because N is small
    std::vector<std::pair<SymbolId, SymbolId>> ctor_map;

public:
    explicit CtorResolver(SymbolPool& sym_pool);

    SymbolId try_apply_rewrite(SymbolId fn) const;
};

} // namespace stc::jl
