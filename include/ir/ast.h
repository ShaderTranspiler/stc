#pragma once

#include <cmath>
#include <cstddef>
#include <iostream>
#include <optional>
#include <tuple>
#include <vector>

#include "common/base.h"
#include "common/concepts.h"
#include "common/src_info.h"
#include "common/utils.h"
#include "ir/types.h"

#define SAME_NODE_T_DEF(Kind)                                                                      \
    static bool same_node_t(const Stmt* stmt) {                                                    \
        return stmt->get_kind() == (Kind);                                                         \
    }

namespace stc::ir {

// CLEANUP: trailing objects pattern where applicable

struct StmtId : public StrongId<uint32_t> {
    using StrongId::StrongId;
};

struct DeclId : public StrongId<uint16_t> {
    using StrongId::StrongId;
};

// ===================
//   Base Node Types
// ===================

// TODO: dyn_cast/same_node_t utils for decls
struct Decl {
    // clang-format off
    enum class DeclKind : uint8_t {
        #define X(decl_type, decl_kind) decl_kind,
            #include "ir/node_defs/decl.def"
        #undef X
    };
    // clang-format on

    // CLEANUP: better packing for decl, string interning

    SrcLocationId location;
    std::string identifier;
    DeclKind kind;

    explicit Decl(SrcLocationId location, DeclKind kind, std::string identifier)
        : location{location}, identifier{std::move(identifier)}, kind{kind} {}

    Decl(const Decl&)            = default;
    Decl(Decl&&)                 = default;
    Decl& operator=(const Decl&) = default;
    Decl& operator=(Decl&&)      = default;

    DeclKind get_kind() const { return kind; }
};

struct Stmt {
    // clang-format off
    enum class NodeKind : uint8_t {
        FirstExpr,

        #define X(type, kind) kind,
        #define X_FIRST(type, kind) kind = FirstExpr,
            #include "node_defs/expr.def"
        #undef X_FIRST

        LastExpr = DeclRef,

        FirstStmt,

        #define X_FIRST(type, kind) kind = FirstStmt,
            #include "node_defs/plain_stmt.def"
        #undef X_FIRST
        #undef X

        LastStmt = Return,
    };
    // clang-format on

    static_assert(sizeof(SrcLocationId) == 4UL);
    static_assert(sizeof(NodeKind) == 1UL);

    SrcLocationId location;
    uint32_t kind         : 8;
    uint32_t type_storage : 16;
    uint32_t node_storage : 8;

    explicit Stmt(SrcLocationId location, NodeKind kind, TypeId type_storage = 0U,
                  uint8_t node_storage = 0U)
        : location{location},
          kind{static_cast<uint32_t>(kind)},
          type_storage{static_cast<uint32_t>(type_storage)},
          node_storage{static_cast<uint32_t>(node_storage)} {}

    Stmt(const Stmt&)            = default;
    Stmt(Stmt&&)                 = default;
    Stmt& operator=(const Stmt&) = default;
    Stmt& operator=(Stmt&&)      = default;

    [[nodiscard]] NodeKind get_kind() const { return static_cast<NodeKind>(kind); }
};

// ================
//   Declarations
// ================

struct VarDecl : public Decl {
    TypeId type;
    std::optional<StmtId> initializer;

    explicit VarDecl(SrcLocationId location, std::string var_name, TypeId type,
                     std::optional<StmtId> initializer = std::nullopt)
        : Decl{location, DeclKind::VarDecl, std::move(var_name)},
          type{type},
          initializer{initializer} {}
};

struct ParamDecl : public Decl {
    TypeId param_type;

    explicit ParamDecl(SrcLocationId location, std::string param_name, TypeId type)
        : Decl{location, DeclKind::ParamDecl, std::move(param_name)}, param_type{type} {}
};

struct FunctionDecl : public Decl {
    TypeId return_type;
    std::vector<DeclId> param_decls;

    explicit FunctionDecl(SrcLocationId location, std::string fn_name, TypeId return_type,
                          std::vector<DeclId> param_decls)
        : Decl{location, DeclKind::FuncDecl, std::move(fn_name)},
          return_type{return_type},
          param_decls{std::move(param_decls)} {}
};

struct FieldDecl : public Decl {
    TypeId field_type;

    explicit FieldDecl(SrcLocationId location, std::string field_name, TypeId field_type)
        : Decl{location, DeclKind::FieldDecl, std::move(field_name)}, field_type{field_type} {}
};

struct StructDecl : public Decl {
    std::vector<DeclId> field_decls;

    explicit StructDecl(SrcLocationId location, std::string struct_name,
                        std::vector<DeclId> field_decls)
        : Decl{location, DeclKind::StructDecl, std::move(struct_name)},
          field_decls{std::move(field_decls)} {}
};

// ===============
//   Expressions
// ===============

struct Expr : public Stmt {
    explicit Expr(SrcLocationId location, NodeKind kind, TypeId type)
        : Stmt{location, kind, type} {}

    explicit Expr(SrcLocationId location, NodeKind kind, TypeId type, uint8_t node_storage)
        : Stmt{location, kind, type, node_storage} {}

    TypeId type() const { return static_cast<TypeId>(type_storage); }

    static bool same_node_t(const Stmt* stmt) {
        return stmt->get_kind() >= NodeKind::FirstExpr && stmt->get_kind() <= NodeKind::LastExpr;
    }
};

struct BoolLiteral : public Expr {
    explicit BoolLiteral(SrcLocationId location, bool value)
        : Expr{location, NodeKind::BoolLit, TypeId::bool_id(), static_cast<uint8_t>(value)} {}

    bool value() const { return static_cast<bool>(node_storage); }

    SAME_NODE_T_DEF(NodeKind::BoolLit)
};

struct IntLiteral : public Expr {
    std::string data;

    explicit IntLiteral(SrcLocationId location, TypeId int_type, std::string data)
        : Expr{location, NodeKind::IntLit, int_type}, data{std::move(data)} {}

    SAME_NODE_T_DEF(NodeKind::IntLit)
};

struct FloatLiteral : public Expr {
    std::string data;

    explicit FloatLiteral(SrcLocationId location, TypeId float_type, std::string data)
        : Expr{location, NodeKind::FloatLit, float_type}, data{std::move(data)} {}

    SAME_NODE_T_DEF(NodeKind::FloatLit)
};

struct VectorLiteral : public Expr {
    std::vector<StmtId> components;

    explicit VectorLiteral(SrcLocationId location, TypeId vec_type, std::vector<StmtId> components)
        : Expr{location, NodeKind::VecLit, vec_type}, components{std::move(components)} {}

    SAME_NODE_T_DEF(NodeKind::VecLit)
};

// column-major storage
struct MatrixLiteral : public Expr {
    std::vector<StmtId> data;

    explicit MatrixLiteral(SrcLocationId location, TypeId mat_type, std::vector<StmtId> data)
        : Expr{location, NodeKind::MatLit, mat_type}, data{std::move(data)} {}

    SAME_NODE_T_DEF(NodeKind::MatLit)
};

struct ArrayLiteral : public Expr {
    std::vector<StmtId> elements;

    explicit ArrayLiteral(SrcLocationId location, TypeId arr_type, std::vector<StmtId> elements)
        : Expr{location, NodeKind::ArrayLit, arr_type}, elements{std::move(elements)} {}

    SAME_NODE_T_DEF(NodeKind::ArrayLit)
};

struct StructInstantiationLiteral : public Expr {
    std::vector<std::pair<std::string, StmtId>> field_values;

    explicit StructInstantiationLiteral(SrcLocationId location, TypeId struct_type,
                                        std::vector<std::pair<std::string, StmtId>> field_values)
        : Expr{location, NodeKind::StructInstLit, struct_type},
          field_values{std::move(field_values)} {}

    SAME_NODE_T_DEF(NodeKind::StructInstLit)
};

struct BinaryOp : public Expr {
    enum class OpKind : uint8_t { add, sub, mul, div, pow, mod };

    StmtId lhs, rhs;

    // CLEANUP: remove need for explicit type
    explicit BinaryOp(SrcLocationId location, TypeId type, OpKind op, StmtId lhs, StmtId rhs)
        : Expr{location, NodeKind::BinOp, type, static_cast<uint8_t>(op)}, lhs{lhs}, rhs{rhs} {}

    OpKind op() const { return static_cast<OpKind>(node_storage); }

    SAME_NODE_T_DEF(NodeKind::BinOp)
};

struct ExplicitCast : public Expr {
    StmtId base;

    explicit ExplicitCast(SrcLocationId location, StmtId base, TypeId target_type)
        : Expr{location, NodeKind::ExplCast, target_type}, base{base} {}

    SAME_NODE_T_DEF(NodeKind::ExplCast)
};

struct DeclRefExpr : public Expr {
    DeclId decl;

    // TODO: remove need for explicit type
    explicit DeclRefExpr(SrcLocationId location, DeclId decl, TypeId decl_type)
        : Expr{location, NodeKind::DeclRef, decl_type}, decl{decl} {}

    SAME_NODE_T_DEF(NodeKind::DeclRef)
};

// ===============
//   Statements
// ===============

struct CompoundStmt : public Stmt {
    std::vector<StmtId> body;

    explicit CompoundStmt(SrcLocationId location, std::vector<StmtId> body)
        : Stmt{location, NodeKind::Compound}, body{std::move(body)} {}

    SAME_NODE_T_DEF(NodeKind::Compound)
};

struct IfStmt : public Stmt {
    StmtId condition_expr;
    StmtId true_block;
    StmtId false_block;

    explicit IfStmt(SrcLocationId location, StmtId condition_expr, StmtId true_block,
                    StmtId false_block)
        : Stmt{location, NodeKind::If},
          condition_expr{condition_expr},
          true_block{true_block},
          false_block{false_block} {}

    SAME_NODE_T_DEF(NodeKind::If)
};

struct ReturnStmt : public Stmt {
    StmtId ret_value_expr;

    explicit ReturnStmt(SrcLocationId location, StmtId ret_value_expr)
        : Stmt{location, NodeKind::Return}, ret_value_expr{ret_value_expr} {}

    SAME_NODE_T_DEF(NodeKind::Return)
};

// =========
//   Utils
// =========

// CLEANUP: introduce concepts for these
template <typename To, typename From>
requires std::is_base_of_v<Stmt, To> && std::is_base_of_v<Stmt, From> && requires (From* ptr) {
    { To::same_node_t(ptr) } -> std::same_as<bool>;
}
To* dyn_cast(From* ptr) {
    if (ptr == nullptr)
        return nullptr;

    if (To::same_node_t(ptr)) {
        return static_cast<To*>(ptr);
    }

    return nullptr;
}

template <typename To, typename From>
requires std::is_base_of_v<Stmt, To> && std::is_base_of_v<Stmt, From> && requires (From* ptr) {
    { To::same_node_t(ptr) } -> std::same_as<bool>;
}
std::unique_ptr<To> dyn_unique_cast(std::unique_ptr<From>&& ptr) {
    if (ptr == nullptr)
        return nullptr;

    if (auto* cast_ptr = dyn_cast<To, From>(ptr.get())) {
        std::ignore = ptr.release();
        return std::unique_ptr<To>{cast_ptr};
    }

    return nullptr;
}

} // namespace stc::ir

#undef DUMP_DECL
#undef DUMP_DECL_NO_OV
#undef DUMP_DECL_PURE
#undef __DUMP_DECL_BASE

#undef SAME_NODE_T_DEF
