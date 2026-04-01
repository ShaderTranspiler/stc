#pragma once

#include "frontend/jl/ast.h"
#include "frontend/jl/context.h"

namespace stc::jl {

enum class BindingType : uint8_t { Global, Local, Captured };

// SymbolId of identifier -> NodeId of corresponding decl
using SymbolTable = std::unordered_map<SymbolId, NodeId>;

using BindingTable = std::unordered_map<SymbolId, BindingType>;

struct JLScope {
    ScopeKind kind;
    CompoundExpr& body;

    // what kind of scopes symbols point to needs to be resolved in a separate pass
    // this is because of Julia's complex scoping rules, which cannot be resolved in a strictly
    // sequential order
    // this info is then used when building the symbol table
    BindingTable binding_table{};
    SymbolTable symbol_table{};

    // needed for deferred method body visits in sema
    std::vector<NodeId> methods{};

    explicit JLScope(ScopeKind kind, CompoundExpr& body)
        : kind{kind}, body{body} {}

    ScopeType type() const {
        return kind == ScopeKind::Global ? ScopeType::Global : ScopeType::Local;
    }

    bool is_global() const { return kind == ScopeKind::Global; } // <==> type() == Global
    bool is_local() const { return kind != ScopeKind::Global; }  // <==> type() == Local
    bool is_hard() const { return kind == ScopeKind::Hard; }
    bool is_soft() const { return kind == ScopeKind::Soft; }

    bool bt_contains(SymbolId sym) const { return binding_table.contains(sym); }

    bool bt_add_sym(SymbolId sym, BindingType binding) {
        return binding_table.try_emplace(sym, binding).second;
    }

    BindingType bt_find_sym(SymbolId sym) const {
        auto it = binding_table.find(sym);

        if (it == binding_table.end())
            throw std::logic_error{"Trying to access scope of symbol not resolved during "
                                   "scope-level symbol resolution"};

        return it->second;
    }

    bool st_contains(SymbolId sym) const { return symbol_table.contains(sym); }

    bool st_add_sym(SymbolId sym, NodeId decl) {
        return symbol_table.try_emplace(sym, decl).second;
    }

    NodeId st_find_sym(SymbolId sym) const {
        auto it = symbol_table.find(sym);

        if (it == symbol_table.end())
            return NodeId::null_id();

        return it->second;
    }

    void defer_method_body_visit(NodeId method_id) {
        if (std::find(methods.begin(), methods.end(), method_id) != methods.end())
            throw std::logic_error{"Trying to defer visitor for method body already in queue"};

        methods.push_back(method_id);
    }

    void dump(const JLCtx& ctx, std::ostream& out = std::cout) const;
};

} // namespace stc::jl