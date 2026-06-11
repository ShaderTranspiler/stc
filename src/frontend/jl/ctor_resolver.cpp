#include "frontend/jl/ctor_resolver.h"

#include <algorithm>
#include <array>

namespace stc::jl {

CtorResolver::CtorResolver(SymbolPool& sym_pool) {
    static constexpr size_t CTOR_FN_COUNT = 15;

    ctor_map.reserve(CTOR_FN_COUNT);

    auto add = [&](std::string_view ctor_fn, std::string_view type) {
        ctor_map.emplace_back(sym_pool.get_id(ctor_fn), sym_pool.get_id(type));
    };

    static constexpr std::array vec_prefixes = {'d', 'i', 'u', 'b'};
    for (uint8_t n = 2; n <= 4; n++) {
        for (auto prefix : vec_prefixes) {
            add(fmt::format("{}vec{}", prefix, n),
                fmt::format("{}Vec{}", static_cast<char>(std::toupper(prefix)), n));
        }

        add(fmt::format("vec{}", n), fmt::format("Vec{}", n));
    }

    // TODO: matrices

    assert(ctor_map.size() == CTOR_FN_COUNT &&
           "CTOR_FN_COUNT doesn't match actual ctor mappings count");

    // sort based on symbol id
    std::ranges::sort(ctor_map, [](const auto& a, const auto& b) { return a.first < b.first; });
}

SymbolId CtorResolver::try_apply_rewrite(SymbolId fn) const {
    auto it = std::lower_bound(ctor_map.begin(), ctor_map.end(), fn,
                               [](const auto& entry, SymbolId id) { return entry.first < id; });

    if (it != ctor_map.end() && it->first == fn)
        return it->second;

    return fn;
}

} // namespace stc::jl
