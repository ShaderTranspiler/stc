#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "common/bump_arena.h"
#include "ir/type_descriptors.h"

namespace stc::ir {

class TypePool final {
private:
    using ArenaT = BumpArena<TypeId>;

public:
    // ! expects arena to not be empty and not used by others (it can only be inspected)
    explicit TypePool(ArenaT& arena)
        : arena{arena} {
        // if these macros ever change, don't forget to modify the default inserts
        static_assert(TypeIds::Void == 0 && TypeIds::Bool == 1);
        std::ignore = arena.emplace<VoidTD>();
        std::ignore = arena.emplace<BoolTD>();
    }

    const TypeDescriptor& get_td(TypeId id) const;

    template <TypeDescriptorT T>
    bool is_type_of(TypeId id) const {
        auto* td = static_cast<TypeDescriptor*>(arena.get_ptr(id));
        assert(td != nullptr);

        return td->is<T>();
    }

    inline TypeId size() const { return static_cast<TypeId>(pool.size()); }

    [[nodiscard]] static TypeId void_td() { return TypeIds::Void; }
    [[nodiscard]] static TypeId bool_td() { return TypeIds::Bool; }
    [[nodiscard]] TypeId int_td(uint32_t width, bool signedness);
    [[nodiscard]] TypeId float_td(uint32_t width, FloatTD::Encoding encoding);
    [[nodiscard]] TypeId vector_td(TypeId component_type_id, uint32_t component_count);
    [[nodiscard]] TypeId matrix_td(TypeId column_type_id, uint32_t column_count);
    [[nodiscard]] TypeId array_td(TypeId element_type_id, uint32_t length);
    [[nodiscard]] TypeId get_struct_td(std::string_view name);

    TypeId make_struct_td(std::string name, std::vector<StructData::FieldInfo> fields);

private:
    [[nodiscard]] TypeId insert(TDVariantType type, bool purge_duplicates = true);

    ArenaT& arena;
    std::unordered_map<TDVariantType, TypeId> pool;
    std::unordered_map<std::string_view, TypeId> struct_map;
};

} // namespace stc::ir
