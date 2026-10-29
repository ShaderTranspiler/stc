#include "backend/glsl/type_utils.h"

#include <fmt/format.h>

namespace stc::glsl {

std::optional<std::string_view> type_prefix(const TypeDescriptor& td) {
    if (td.is<BoolTD>())
        return "b";

    if (td.is<IntTD>()) {
        const auto& int_td = td.as<IntTD>();
        if (int_td.width != 32)
            return std::nullopt;

        return int_td.is_signed ? "i" : "u";
    }

    if (td.is<FloatTD>()) {
        const auto& float_td = td.as<FloatTD>();
        if (float_td.width != 32 && float_td.width != 64)
            return std::nullopt;

        return float_td.width == 32 ? "" : "d";
    }

    return std::nullopt;
}

std::optional<std::string> type_str(const TypeDescriptor& td, const TypePool& type_pool,
                                    const SymbolPool& sym_pool) {
    if (td.is<VoidTD>())
        return "void";

    if (td.is<BoolTD>())
        return "bool";

    if (td.is<IntTD>()) {
        auto [width, is_signed] = td.as<IntTD>();
        if (width != 32)
            return std::nullopt;

        return is_signed ? "int" : "uint";
    }

    if (td.is<FloatTD>()) {
        auto [width, enc] = td.as<FloatTD>();
        if (enc != FloatTD::Encoding::ieee754 || (width != 32 && width != 64))
            return std::nullopt;

        return width == 32 ? "float" : "double";
    }

    if (td.is_vector()) {
        auto [comp_type_id, size] = td.as<VectorTD>();
        const auto& comp_td       = type_pool.get_td(comp_type_id);
        assert(comp_td.is_scalar() && "non-scalar vector component type");

        auto prefix = type_prefix(comp_td);
        if (!prefix.has_value())
            return std::nullopt;

        return std::string{*prefix} + "vec" + std::to_string(size);
    }

    if (td.is_matrix()) {
        auto [col_type_id, col_count] = td.as<MatrixTD>();
        const auto& col_td            = type_pool.get_td(col_type_id);
        assert(col_td.is_vector() && "non-vector matrix column type");

        auto [comp_type_id, row_count] = col_td.template as<VectorTD>();
        const auto& comp_td            = type_pool.get_td(comp_type_id);
        assert(comp_td.template is<FloatTD>() && "non-floating point matrix component type");

        auto prefix = type_prefix(comp_td);
        if (!prefix.has_value())
            return std::nullopt;

        return fmt::format("{}mat{}x{}", *prefix, col_count, row_count);
    }

    if (td.is_array()) {
        std::string dims_str{};
        dims_str.reserve(16);

        const TypeDescriptor* it_td = &td;
        do {
            uint32_t len     = it_td->as<ArrayTD>().length;
            bool is_any_size = len == std::numeric_limits<uint32_t>::max();
            dims_str += fmt::format("[{}]", !is_any_size ? std::to_string(len) : "");

            auto [elem_type_id, dim_length] = it_td->as<ArrayTD>();

            it_td = &type_pool.get_td(elem_type_id);
        } while (it_td->is_array());

        auto partial_type_str = type_str(*it_td, type_pool, sym_pool);
        if (!partial_type_str.has_value())
            return std::nullopt;

        return *partial_type_str + dims_str;
    }

    if (td.is_struct()) {
        auto [data_ptr] = td.as<StructTD>();
        assert(data_ptr != nullptr && "StructTD without struct data");

        return std::string{sym_pool.get_symbol(data_ptr->name)};
    }

    return std::nullopt;
}

std::optional<std::string> decl_str(const TypeDescriptor& td, const TypePool& type_pool,
                                    const SymbolPool& sym_pool, SymbolId name) {
    if (td.is_array()) {
        std::string dims_str{};
        dims_str.reserve(16);

        const TypeDescriptor* it_td = &td;
        do {
            uint32_t len     = it_td->as<ArrayTD>().length;
            bool is_any_size = len == std::numeric_limits<uint32_t>::max();
            dims_str += fmt::format("[{}]", !is_any_size ? std::to_string(len) : "");

            auto [elem_type_id, dim_length] = it_td->as<ArrayTD>();

            it_td = &(type_pool.get_td(elem_type_id));
        } while (it_td->is_array());

        auto type_str_result = type_str(*it_td, type_pool, sym_pool);
        if (!type_str_result.has_value())
            return std::nullopt;

        return fmt::format("{} {}{}", *type_str_result, sym_pool.get_symbol(name), dims_str);
    }

    auto type_str_result = type_str(td, type_pool, sym_pool);
    if (!type_str_result.has_value())
        return std::nullopt;

    return fmt::format("{} {}", *type_str_result, sym_pool.get_symbol(name));
}

} // namespace stc::glsl
