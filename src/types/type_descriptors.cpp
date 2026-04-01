#include <cassert>
#include <format>

#include "ast/context.h"
#include "types/type_descriptors.h"
#include "types/type_pool.h"

namespace {

using FPEnc = stc::types::FloatTD::Encoding;

std::string to_string(FPEnc enc) {
    switch (enc) {
        case FPEnc::ieee754:
            return "IEEE-754";

        case FPEnc::bfloat16:
            return "bfloat16";

        case FPEnc::f8e4m3:
            return "F8 E4/M3";

        case FPEnc::f8e5m2:
            return "F8 E5/M2";
    }

    throw std::logic_error{"Unaccounted floating point encoding"};
}

} // namespace

namespace stc::types {

MatrixTD::MatrixInfo MatrixTD::get_info(TypeId mat_id, const TypePool& type_pool) {
    assert(type_pool.is_type_of<MatrixTD>(mat_id) && "mat_id points to non-matrix type");
    MatrixTD mat_td = type_pool.get_td(mat_id).as<MatrixTD>();

    assert(type_pool.is_type_of<VectorTD>(mat_td.column_type_id) &&
           "non-vector column type in matrix");
    VectorTD vec_td = type_pool.get_td(mat_td.column_type_id).as<VectorTD>();

    return {.rows           = vec_td.component_count,
            .cols           = mat_td.column_count,
            .component_type = vec_td.component_type_id};
}

std::vector<uint32_t> ArrayTD::get_dims(TypeId arr_id, const TypePool& type_pool) {
    assert(type_pool.is_type_of<ArrayTD>(arr_id) && "arr_id points to non-array type");

    std::vector<uint32_t> dims{};

    const TypeDescriptor* it_td = &type_pool.get_td(arr_id);
    do {
        ArrayTD it_arr_td = it_td->as<ArrayTD>();
        dims.push_back(it_arr_td.length);

        it_td = &type_pool.get_td(it_arr_td.element_type_id);
    } while (it_td->is_array());

    return dims;
}

bool StructTD::operator==(const StructTD& other) const {
    if (data == other.data)
        return true;

    if (data == nullptr || other.data == nullptr)
        return false;

    return *data == *other.data;
}

bool MethodTD::operator==(const MethodTD& other) const {
    if (sig == other.sig)
        return true;

    if (sig == nullptr || other.sig == nullptr)
        return false;

    return *sig == *other.sig;
}

bool TypeDescriptor::operator==(const TypeDescriptor& other) const {
    return _type_data.index() == other._type_data.index() && _type_data == other._type_data;
}

std::string to_string(const TypeDescriptor& type, const TypePool& type_pool,
                      const SymbolPool& sym_pool) {
    auto visitor = [&type_pool, &sym_pool](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, VoidTD>) {
            return "void";
        } else if constexpr (std::is_same_v<T, BoolTD>) {
            return "bool";
        } else if constexpr (std::is_same_v<T, IntTD>) {
            return std::format("{}i{}", arg.is_signed ? "" : "u", arg.width);
        } else if constexpr (std::is_same_v<T, FloatTD>) {
            return std::format("f{}{}{}", arg.width, FloatTD::required_width(arg.enc) ? "!" : "",
                               arg.enc == FloatTD::Encoding::ieee754
                                   ? ""
                                   : std::format(" ({})", ::to_string(arg.enc)));
        } else if constexpr (std::is_same_v<T, VectorTD>) {
            return std::format(
                "vec{}<{}>", arg.component_count,
                to_string(type_pool.get_td(arg.component_type_id), type_pool, sym_pool));
        } else if constexpr (std::is_same_v<T, MatrixTD>) {
            return std::format(
                "matrix ({}x {})", arg.column_count,
                to_string(type_pool.get_td(arg.column_type_id), type_pool, sym_pool));
        } else if constexpr (std::is_same_v<T, ArrayTD>) {
            const TypeDescriptor& el_type = type_pool.get_td(arg.element_type_id);
            return std::format("array[{}] of {}", arg.length,
                               std::holds_alternative<ArrayTD>(el_type.type_data())
                                   ? std::format("({})", to_string(el_type, type_pool, sym_pool))
                                   : to_string(el_type, type_pool, sym_pool));
        } else if constexpr (std::is_same_v<T, StructTD>) {
            return std::format("struct {}", sym_pool.get_symbol(arg.data->name));
        } else if constexpr (std::is_same_v<T, MethodTD>) {
            if (arg.sig == nullptr)
                return "fn ? -> ?";

            std::string param_list{};
            for (size_t i = 0; i < arg.sig->param_types.size(); i++) {
                if (i > 0)
                    param_list += ", ";

                param_list += to_string(arg.sig->param_types[i], type_pool, sym_pool);
            }

            return std::format("method ({}) -> {}", param_list, "asd");
        } else if constexpr (std::is_same_v<T, FunctionTD>) {
            auto sym = sym_pool.get_symbol_maybe(arg.identifier);
            return std::format("fn '{}'", sym.value_or("invalid symbol id"));
        } else if constexpr (std::is_same_v<T, BuiltinTD>) {
            // TODO
            return "?";
        } else {
            static_assert(false, "missing type descriptor case(s) in visitor");
        }
    };

    return std::visit(visitor, type.type_data());
}

std::string to_string(TypeId type_id, const TypePool& type_pool, const SymbolPool& sym_pool) {
    if (type_id.is_null())
        return "unknown type";

    return to_string(type_pool.get_td(type_id), type_pool, sym_pool);
}

} // namespace stc::types
