#pragma once

#include "ir/ast_context.h"
#include <type_traits>

namespace stc::ir {

#define STC_AST_VISITOR_DECL(RetTy, type) RetTy visit_##type(type&);

template <typename ImplTy, typename RetTy>
concept ASTVisitorImpl = requires (ImplTy v) {

#define X(type, kind) {v.visit_##type(std::declval<type&>())}->std::same_as<RetTy>;
#include "ir/node_defs/decl.def"
#include "ir/node_defs/stmt.def"
#undef X
};

// CRTP-style base AST visitor class
// derived visitors need to implement a visit_T function for every Stmt and Decl in the AST
// ir/node_defs/decl.def and ir/node_defs/stmt.def can be used with the X-macro system to
// automatically generate declarations for these, as seen below
// declarations in the base class have been commented out, as both cases lead to the same kind of
// linker error, when not implemented properly (these errors contain the exact member functions
// that are missing)
// when declarations aren't generated automatically, a static_assert on the ASTVisitorImpl concept
// can be used to turn linker errors into compile-time concept-based errors (with automatic
// declarations, this will pass and errors will only be generated at link-time)
template <typename ImplTy, typename RetTy = void>
class ASTVisitor {
protected:
    ASTCtx& ctx;

    explicit ASTVisitor(ASTCtx& ast_context)
        : ctx{ast_context} {}

private:
    ImplTy* impl_this() { return static_cast<ImplTy*>(this); }
    const ImplTy* impl_this() const { return static_cast<const ImplTy*>(this); }

    template <typename T, typename U>
    static T& as(U* ptr) {
        return *static_cast<T*>(ptr);
    }

public:
    RetTy visit(StmtId id) {
        impl_this()->pre_visit_stmt(id);

        Stmt* stmt = ctx.get_stmt(id);
        return dispatch_stmt(stmt);
    }

    RetTy visit(DeclId id) {
        impl_this()->pre_visit_decl(id);

        Decl* decl = ctx.get_decl(id);
        return dispatch_decl(decl);
    }

    RetTy dispatch_stmt(Stmt* stmt) {
        if (stmt == nullptr)
            return impl_this()->visit_default_case();

        // clang-format off
        switch (stmt->get_kind()) {
            #define X(type, kind)                                                                  \
                case (Stmt::NodeKind::kind):                                                       \
                    return impl_this()->visit_##type(as<type>(stmt));

                #include "ir/node_defs/stmt.def"
            #undef X

            default:
                throw std::logic_error{"Missing Stmt case in ASTVisitor"};
        }
        // clang-format on
    }

    RetTy dispatch_decl(Decl* decl) {
        if (decl == nullptr)
            return impl_this()->visit_default_case();

        // clang-format off
        switch (decl->get_kind()) {
            #define X(type, kind)                                                                  \
                case (Decl::DeclKind::kind):                                                       \
                    return impl_this()->visit_##type(as<type>(decl));

                #include "ir/node_defs/decl.def"
            #undef X

            default:
                throw std::logic_error{"Missing Decl case in ASTVisitor"};
        }
        // clang-format on
    }

    RetTy visit_default_case() {
        if constexpr (!std::is_void_v<RetTy>) {
            static_assert(
                dependent_false_v<RetTy>,
                "Non-void returning ASTVisitors must define fallback logic in visit_default_case");
        }
    }

    void pre_visit_stmt(StmtId) {}
    void pre_visit_decl(DeclId) {}

    // #define X(type, kind) STC_AST_VISITOR_DECL(RetTy, type)
    //     #include "ir/node_defs/decl.def"
    //     #include "ir/node_defs/stmt.def"
    // #undef X
};

} // namespace stc::ir
