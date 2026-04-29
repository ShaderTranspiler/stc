#include "backend/glsl/type_utils.h"
#include "ast/context.h"

#include <fmt/format.h>

namespace stc::glsl {

std::string type_str(const TypeDescriptor& td, const TypePool& type_pool,
                     const SymbolPool& sym_pool) {
    if (td.is<VoidTD>())
        return "void";

    if (td.is<BoolTD>())
        return "bool";

    if (td.is<IntTD>()) {
        auto [width, is_signed] = td.as<IntTD>();
        assert((width == 32 || width == 64) && "invalid glsl int width");

        return is_signed ? "int" : "uint";
    }

    if (td.is<FloatTD>()) {
        auto [width, _enc] = td.as<FloatTD>();
        assert((width == 32 || width == 64) && "invalid glsl floating point width");

        return width == 32 ? "float" : "double";
    }

    if (td.is_vector()) {
        auto [comp_type_id, size] = td.as<VectorTD>();
        const auto& comp_td       = type_pool.get_td(comp_type_id);
        assert(comp_td.is_scalar() && "non-scalar vector component type");

        return type_prefix(comp_td) + "vec" + std::to_string(size);
    }

    if (td.is_matrix()) {
        auto [col_type_id, col_count] = td.as<MatrixTD>();
        const auto& col_td            = type_pool.get_td(col_type_id);
        assert(col_td.is_vector() && "non-vector matrix column type");

        auto [comp_type_id, row_count] = col_td.template as<VectorTD>();
        const auto& comp_td            = type_pool.get_td(comp_type_id);
        assert(comp_td.template is<FloatTD>() && "non-floating point matrix component type");

        return fmt::format("{}mat{}x{}", type_prefix(comp_td), col_count, row_count);
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

            it_td = &(type_pool.get_td(elem_type_id));
        } while (it_td->is_array());

        return type_str(*it_td, type_pool, sym_pool) + dims_str;
    }

    if (td.is_struct()) {
        auto [data_ptr] = td.as<StructTD>();
        assert(data_ptr != nullptr && "StructTD without struct data");

        return std::string{sym_pool.get_symbol(data_ptr->name)};
    }

    if (td.is_function() || td.is_method()) {
        error("trying to get GLSL string representation for function or method type");
        return "?";
    }

    if (td.is_builtin()) {
        error("trying to get GLSL string representation for non-GLSL builtin type");
        return "?";
    }

    assert(false && "missing type case in glsl code gen's type_str");
    error("trying to get GLSL string representation for unknown type kind");
    return "?";
}

std::string decl_str(const TypeDescriptor& td, const TypePool& type_pool,
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

        return fmt::format("{} {}{}", type_str(*it_td, type_pool, sym_pool),
                           sym_pool.get_symbol(name), dims_str);
    }

    return fmt::format("{} {}", type_str(td, type_pool, sym_pool), sym_pool.get_symbol(name));
}

} // namespace stc::glsl
