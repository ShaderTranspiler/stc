#pragma once

#include "common/bump_arena.hpp"
#include "common/dyn_cast.hpp"
#include "common/src_info.hpp"
#include "common/target_info.hpp"
#include "types/qualifier_pool.hpp"
#include "types/type_pool.hpp"

namespace stc {

// base AST context class

// encapsulates data needed for any sort of handling of an AST, regardless of its type or
// implementation

// NodeIdTy is the size type used to store arena indices/offsets when referring to nodes
// NodeBaseTy is the base class from which all AST types are derived
// Derived should be provided by classes deriving from this, so that partial movability can be
// ensured automatically at compile-time through concepts

template <CNullableSplitId NodeIdTy, typename NodeBaseTy>
requires requires (NodeBaseTy node, NodeIdTy id, void* void_ptr) {
    typename NodeBaseTy::kind_type;
    { node.kind() } -> std::same_as<typename NodeBaseTy::kind_type>;
    { NodeBaseTy::safe_cast_to_base(void_ptr, id) } -> std::same_as<NodeBaseTy*>;
} && CEnumOf<typename NodeBaseTy::kind_type, typename NodeIdTy::kind_type>
class ASTCtx {
public:
    using node_id_type   = NodeIdTy;
    using node_base_type = NodeBaseTy;
    using node_kind_type = NodeBaseTy::kind_type;

    TranspilerConfig config{};
    const TargetInfo* target_info; // MIGHT be nullptr (transpilation to SIR should be supported)

protected:
    BumpArena<typename NodeIdTy::id_type> node_arena;

public:
    std::string node_pool_name;
    types::TypePool type_pool;
    SrcInfoPool src_info_pool;
    SymbolPool sym_pool;
    QualifierPool qual_pool;

protected:
    size_t tmp_sym_counter = 0U;
    size_t usym_counter    = 0U;

public:
    // all arena sizes are in bytes
    explicit ASTCtx(
        NodeIdTy::id_type initial_node_arena_size,
        SrcLocationId::id_type initial_src_info_arena_size,
        types::TypeId::id_type initial_type_arena_size, SymbolId::id_type initial_symbol_arena_size,
        QualId::id_type initial_qualifier_arena_size,
        // ref of temporary here is fine, since type_pool has to copy its members
        const std::vector<types::BuiltinTD>& type_builtins = std::vector<types::BuiltinTD>{},
        const TargetInfo* target_info = nullptr, std::string node_pool_name = "Node Pool")
        : target_info{target_info},
          node_arena{initial_node_arena_size},
          node_pool_name{std::move(node_pool_name)},
          type_pool{initial_type_arena_size, type_builtins},
          src_info_pool{initial_src_info_arena_size},
          sym_pool{initial_symbol_arena_size},
          qual_pool{initial_qualifier_arena_size} {}

    ~ASTCtx() = default;

    ASTCtx(const ASTCtx&)                  = delete;
    ASTCtx& operator=(const ASTCtx&) const = delete;
    ASTCtx(ASTCtx&&) noexcept              = default;
    ASTCtx& operator=(ASTCtx&&) noexcept   = default;

    // safe to call on a partially moved context
    ErasedArenaView get_node_pool_erased_arena_view() const {
        return {
            .name    = node_pool_name,
            .arena   = &node_arena,
            .size_id = ErasedArenaView::get_size_id<typename NodeIdTy::id_type>(),
        };
    }

    // ! use with caution
    NodeIdTy calculate_node_id(const void* ptr, node_kind_type kind) const {
        return NodeIdTy{node_arena.get_offset(ptr), static_cast<uint8_t>(kind)};
    }

    // ! use with caution
    NodeIdTy calculate_node_id(const node_base_type& expr) const {
        return calculate_node_id(&expr, expr.kind());
    }

protected:
    // "partial" move ctor, moving the type and src info pools, and empty initializing node arena
    // protected to allow derived classes to use as a starting point, but publicly only accessible
    // from the factory function move_pools_from
    // it copies node_pool_name instead of moving so that get_node_pool_erased_arena_view remains
    // safe to call on a partially moved context
    template <typename T, typename U>
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    explicit ASTCtx(ASTCtx<T, U>&& other, NodeIdTy::id_type initial_node_arena_size)
        : config{std::move(other.config)},
          target_info{other.target_info},
          node_arena{initial_node_arena_size},
          node_pool_name{other.node_pool_name},
          type_pool{std::move(other.type_pool)},
          src_info_pool{std::move(other.src_info_pool)},
          sym_pool{std::move(other.sym_pool)},
          qual_pool{std::move(other.qual_pool)},
          tmp_sym_counter{other.get_tmp_sym_counter()},
          usym_counter{other.get_usym_counter()} {}

public:
    // initial_node_arena_size is in bytes
    template <typename T, typename U>
    [[nodiscard]] static ASTCtx move_pools_from(ASTCtx<T, U>&& other,
                                                NodeIdTy::id_type initial_node_arena_size) {
        return ASTCtx{std::move(other), initial_node_arena_size};
    }

    template <typename T, typename... Args>
    requires std::derived_from<T, NodeBaseTy> && requires (T t) {
        { t.kind() } -> std::same_as<node_kind_type>;
    }
    [[nodiscard]] std::pair<NodeIdTy, T*> emplace_node(Args&&... args) {
        auto [offset, ptr] = node_arena.template emplace<T>(std::forward<Args>(args)...);

        if ((offset & NodeIdTy::KIND_MASK) != 0)
            throw std::overflow_error{"AST node id type exhausted"};

        return {NodeIdTy{offset, static_cast<NodeIdTy::kind_type>(ptr->kind())}, ptr};
    }

    [[nodiscard]] NodeBaseTy* get_node(NodeIdTy id) const {
        if (id.is_null())
            return nullptr;

        return NodeBaseTy::safe_cast_to_base(node_arena.get_ptr(id.id_value()), id);
    }

    // CLEANUP: get_and_dyn_cast should be able to skip base cast when T is the concrete node type
    template <typename T>
    requires CDynCastable<T, NodeBaseTy>
    [[nodiscard]] T* get_and_dyn_cast(NodeIdTy id) const {
        // below is essentially the same as: return dyn_cast<T*>(get_node(id));
        // but skips the round-trip to NodeBaseTy* for non-castable cases

        if (!isa<T>(id))
            return nullptr;

        return static_cast<T*>(get_node(id));
    }

    template <typename... T>
    requires (CDynCastable<T, NodeBaseTy> && ...)
    bool isa(NodeIdTy id) const {
        if (id.is_null())
            return false;

        return (T::same_node_kind(static_cast<node_kind_type>(id.kind_value())) || ...);
    }

    [[nodiscard]] std::string_view get_sym(SymbolId sym_id) const {
        auto result = sym_pool.get_symbol_maybe(sym_id);

        if (!result.has_value())
            throw std::logic_error{"symbol id not found in current context state"};

        return *result;
    }

    [[nodiscard]] const DeclQualifiers& get_quals(QualId qual_id) const {
        return qual_pool.get_quals(qual_id);
    }

    operator const types::TypePool&() const { return type_pool; }
    operator const SrcInfoPool&() const { return src_info_pool; }
    operator const SymbolPool&() const { return sym_pool; }

    size_t get_usym_counter() const { return usym_counter; }
    size_t get_tmp_sym_counter() const { return tmp_sym_counter; }

    // note that this is meant to be used post-parser where all user symbols are all already in the
    // symbol pool (so that we can skip them)
    SymbolId get_usym_for(SymbolId sym_id) {
        if (sym_id.is_null())
            throw std::logic_error{"Trying to generate usym for null symbol"};

        auto sym_str = sym_pool.get_symbol_maybe(sym_id);
        if (!sym_str.has_value())
            throw std::logic_error{"Trying to generate usym for invalid SymbolId"};

        // skip suffixes that come from the end user (otherwise collision between mangled and
        // unmangled symbols may occur)
        std::string usym_name{};
        do {
            usym_name = fmt::format("{}_{}", *sym_str, usym_counter++);
        } while (sym_pool.contains(usym_name));

        return sym_pool.get_id(usym_name);
    }

    SymbolId get_tmp_sym_for(SymbolId sym_id) {
        return wrap_sym(sym_id, fmt::format("_stc_tmp_{}", tmp_sym_counter++));
    }

    SymbolId get_tmp_sym() { return sym_pool.get_id(fmt::format("stc_tmp_{}", tmp_sym_counter++)); }

private:
    SymbolId wrap_sym(SymbolId sym_id, std::string_view suffix) {
        if (sym_id.is_null())
            throw std::logic_error{"Trying to wrap null symbol"};

        auto sym_str = sym_pool.get_symbol_maybe(sym_id);
        if (!sym_str.has_value())
            throw std::logic_error{"Trying to wrap sym for symbol not found in pool"};

        std::string wrapped_sym_str{fmt::format("{}{}", *sym_str, suffix)};

        return sym_pool.get_id(wrapped_sym_str);
    }
};

}; // namespace stc
