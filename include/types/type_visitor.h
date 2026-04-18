#pragma once

#include "types/type_pool.h"

#include <type_traits>

namespace stc::types {

template <typename ImplTy, typename RetTy>
concept CHasValuelessTypeVisitor = requires (ImplTy impl) {
    { impl.visit_valueless() } -> std::convertible_to<RetTy>;
};

template <typename ImplTy, typename RetTy>
concept CHasNullIdTypeVisitor = requires (ImplTy impl) {
    { impl.visit_null_id() } -> std::convertible_to<RetTy>;
};

template <typename ImplTy, typename RetTy = void>
class TypeVisitor {
protected:
    const TypePool& type_pool;

    ImplTy* impl_this() { return static_cast<ImplTy*>(this); }
    const ImplTy* impl_this() const { return static_cast<const ImplTy*>(this); }

    explicit TypeVisitor(const TypePool& type_pool)
        : type_pool{type_pool} {}

    // TDVariantType dispatch is not public API, so that implementations can expect the visitor to
    // be called at least at the TypeDescriptor-level, providing some type pool contextso
    RetTy dispatch(TDVariantType type) {
        if (type.valueless_by_exception()) {
            if constexpr (CHasValuelessTypeVisitor<ImplTy, RetTy>)
                return impl_this()->visit_valueless();
            else
                throw std::logic_error{"valueless variant passed to type visitor"};
        }

        return std::visit(
            [this](auto&& type_val) -> RetTy {
                // CLEANUP: this static assert would be nice but fails on msvc for access reasons

                // using T = std::decay_t<decltype(type_val)>;
                // static_assert(CHasVisitorFor<ImplTy, RetTy, T>,
                //               "missing visit overload in type visitor implementation");

                return impl_this()->visit(type_val);
            },
            type);
    }

public:
    RetTy dispatch(const TypeDescriptor& td) { return impl_this()->dispatch(td.type_data()); }

    RetTy dispatch(TypeId id) {
        if (id.is_null()) {
            if constexpr (CHasNullIdTypeVisitor<ImplTy, RetTy>)
                return impl_this()->visit_null_id();
            else
                throw std::logic_error{"null id passed to type visitor"};
        }

        return impl_this()->dispatch(type_pool.get_td(id));
    }
};

// helper macro for type visitor headers
#define STC_TYPE_VISITOR_DECLS(ret_type)                                                           \
    ret_type visit(VoidTD);                                                                        \
    ret_type visit(BoolTD);                                                                        \
    ret_type visit(IntTD);                                                                         \
    ret_type visit(FloatTD);                                                                       \
    ret_type visit(ArrayTD);                                                                       \
    ret_type visit(VectorTD);                                                                      \
    ret_type visit(MatrixTD);                                                                      \
    ret_type visit(StructTD);                                                                      \
    ret_type visit(MethodTD);                                                                      \
    ret_type visit(FunctionTD);                                                                    \
    ret_type visit(BuiltinTD);

} // namespace stc::types
