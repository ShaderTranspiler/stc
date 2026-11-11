#include "frontend/jl/op_resolver.hpp"

#include <algorithm>

namespace stc::jl {

OpResolver::OpResolver(SymbolPool& sym_pool) {
    static constexpr size_t OP_COUNT = 26;

    auto add = [&](std::string_view op_str, TargetOpKind unop, TargetOpKind binop) {
        assert(unop == OpMapping::INVALID_OP ||
               (TargetOpKind::FirstUnop <= unop && unop <= TargetOpKind::LastUnop));
        assert(binop == OpMapping::INVALID_OP ||
               (TargetOpKind::FirstBinop <= binop && binop <= TargetOpKind::LastBinop));

        op_map.emplace_back(sym_pool.get_id(op_str), OpMapping{unop, binop});
    };

    auto add_unop = [&](std::string_view op_str, TargetOpKind unop) {
        add(op_str, unop, OpMapping::INVALID_OP);
    };

    auto add_binop = [&](std::string_view op_str, TargetOpKind binop) {
        add(op_str, OpMapping::INVALID_OP, binop);
    };

    op_map.reserve(OP_COUNT);

    add("+", TargetOpKind::UnopPlus, TargetOpKind::BinopAdd);
    add("-", TargetOpKind::UnopNeg, TargetOpKind::BinopSub);

    // pre/post inc/dec is not supported in Julia

    add_unop("!", TargetOpKind::UnopLogNot);
    add_unop("~", TargetOpKind::UnopBitNeg);

    add_binop("*", TargetOpKind::BinopMul);

    add_binop("/", TargetOpKind::BinopDiv);
    add_binop("div", TargetOpKind::BinopDiv);
    add_binop("÷", TargetOpKind::BinopDiv);

    add_binop("%", TargetOpKind::BinopMod);
    add_binop("rem", TargetOpKind::BinopMod);
    add_binop("mod", TargetOpKind::BinopMod);

    add_binop("<", TargetOpKind::BinopLt);
    add_binop("<=", TargetOpKind::BinopLeq);
    add_binop(">", TargetOpKind::BinopGt);
    add_binop(">=", TargetOpKind::BinopGeq);
    add_binop("==", TargetOpKind::BinopEq);
    add_binop("!=", TargetOpKind::BinopNeq);

    add_binop("&&", TargetOpKind::BinopLogAnd);
    add_binop("||", TargetOpKind::BinopLogOr);

    add_binop("&", TargetOpKind::BinopBitAnd);
    add_binop("|", TargetOpKind::BinopBitOr);
    add_binop("xor", TargetOpKind::BinopBitXor);
    add_binop("⊻", TargetOpKind::BinopBitXor);

    add_binop("<<", TargetOpKind::BinopLShift);
    add_binop(">>", TargetOpKind::BinopRShift);
    add_binop(">>>", TargetOpKind::BinopRShift);

    assert(op_map.size() == OP_COUNT && "OP_COUNT doesn't match actual operator count");

    // sort based on symbol id
    std::ranges::sort(op_map, [](const auto& a, const auto& b) { return a.first < b.first; });
}

TargetOpKind OpResolver::get_target_op(SymbolId op_sym, bool is_binop) const {
    auto it = std::lower_bound(op_map.begin(), op_map.end(), op_sym,
                               [](const auto& entry, SymbolId id) { return entry.first < id; });

    if (it != op_map.end() && it->first == op_sym)
        return is_binop ? it->second.binop : it->second.unop;

    return OpMapping::INVALID_OP;
}

} // namespace stc::jl
