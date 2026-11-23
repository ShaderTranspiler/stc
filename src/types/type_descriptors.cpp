#include <cassert>
#include <fmt/format.h>

#include "ast/context.hpp"
#include "types/type_descriptors.hpp"
#include "types/type_pool.hpp"

namespace stc::types {

MatrixTD::MatrixInfo MatrixTD::get_info(const TypePool& type_pool) const {
    assert(type_pool.is_type_of<VectorTD>(column_type_id) && "non-vector column type in matrix");
    VectorTD vec_td = type_pool.get_td(column_type_id).as<VectorTD>();

    return {.rows           = vec_td.component_count,
            .cols           = column_count,
            .component_type = vec_td.component_type_id};
}

MatrixTD::MatrixInfo MatrixTD::get_info(TypeId mat_id, const TypePool& type_pool) {
    assert(type_pool.is_type_of<MatrixTD>(mat_id) && "mat_id points to non-matrix type");
    MatrixTD mat_td = type_pool.get_td(mat_id).as<MatrixTD>();

    return mat_td.get_info(type_pool);
}

TypeId MatrixTD::get_el_type(const TypePool& type_pool) const {
    assert(type_pool.is_type_of<VectorTD>(column_type_id) && "non-vector column type in matrix");
    VectorTD vec_td = type_pool.get_td(column_type_id).as<VectorTD>();

    return vec_td.component_type_id;
}

TypeId MatrixTD::get_el_type(TypeId mat_id, const TypePool& type_pool) {
    assert(type_pool.is_type_of<MatrixTD>(mat_id) && "mat_id points to non-matrix type");
    MatrixTD mat_td = type_pool.get_td(mat_id).as<MatrixTD>();

    return mat_td.get_el_type(type_pool);
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

} // namespace stc::types
