#pragma once

#include "types/type_descriptors.h"

#include <optional>
#include <unordered_map>

namespace stc {

using namespace stc::types;

enum class TargetOpKind : uint8_t {
    FirstUnop = 0,
    UnopPlus  = FirstUnop,
    UnopNeg,
    UnopPreInc,
    UnopPostInc,
    UnopPreDec,
    UnopPostDec,
    UnopLogNot,
    UnopBitNeg,
    LastUnop = UnopBitNeg,
    FirstBinop,
    BinopAdd = FirstBinop,
    BinopSub,
    BinopMul,
    BinopDiv,
    BinopMod,
    BinopGt,
    BinopGeq,
    BinopLt,
    BinopLeq,
    BinopEq,
    BinopNeq,
    BinopLogAnd,
    BinopLogOr,
    BinopLogXor,
    BinopBitAnd,
    BinopBitOr,
    BinopBitXor,
    BinopLShift,
    BinopRShift,
    LastBinop = BinopRShift,
};

// new target (backend) languages should register themselves here
// this allows language-specific rewrites to be enabled in early transpiler passes
// (though that should be done with care)
enum class TargetLanguage : uint8_t { GLSL };

// serves as an "oracle" for backend capabilities in early passes, to keep things as
// backend-agnostic as possible
class TargetInfo {
protected:
    using TypeList = std::vector<TypeId>;

    // string_view-s are used in these structs since these are only meant to be used with string
    // literals everywhere

    struct BuiltinGlobal {
        std::string_view name;
        TypeId type;
    };
    using GlobalList = std::vector<BuiltinGlobal>;

    struct BuiltinFnOverload {
        TypeList arg_types;
        TypeId ret_type;
    };
    using OverloadList = std::vector<BuiltinFnOverload>;

    struct BuiltinFunction {
        std::string_view name;
        OverloadList overloads;

        BuiltinFunction(std::string_view name, OverloadList overloads)
            : name{name}, overloads{std::move(overloads)} {}
    };
    using FnList = std::vector<BuiltinFunction>;

    TargetLanguage language_id;
    GlobalList globals;
    FnList functions;

public:
    explicit TargetInfo(TargetLanguage language_id, GlobalList builtin_globals, FnList builtin_fns)
        : language_id{language_id},
          globals{std::move(builtin_globals)},
          functions{std::move(builtin_fns)} {

        assert(std::is_sorted(globals.begin(), globals.end(),
                              [](const BuiltinGlobal& a, const BuiltinGlobal& b) {
                                  return a.name < b.name;
                              }) &&
               "unsorted globals list in backend info");

        assert(std::is_sorted(functions.begin(), functions.end(),
                              [](const BuiltinFunction& a, const BuiltinFunction& b) {
                                  return a.name < b.name;
                              }) &&
               "unsorted functions list in backend info");
    }

    virtual ~TargetInfo() = default;

    TargetInfo(const TargetInfo&)            = default;
    TargetInfo& operator=(const TargetInfo&) = default;
    TargetInfo(TargetInfo&&)                 = default;
    TargetInfo& operator=(TargetInfo&&)      = default;

    TargetLanguage language() const { return language_id; }

    bool has_builtin_fn(std::string_view fn_name) const;
    bool has_builtin_fn(std::string_view fn_name, const TypeList& arg_types,
                        bool allow_impl_cast = false) const;

    TypeId builtin_fn_ret_ty(std::string_view fn_name, const TypeList& arg_types) const;

    // returns the returned type id, and the signature the args need to be implicit cast to
    std::pair<TypeId, const TypeList&>
    builtin_fn_ret_ty_with_impl_cast(std::string_view fn_name, const TypeList& arg_types) const;

    bool has_builtin_global(std::string_view global_name) const;
    TypeId builtin_global_ty(std::string_view global_name) const;

    // returns null id if the given op + sig pair is not valid
    // whether it applies implicit casting is up to target rules
    virtual TypeId builtin_op_ret_ty(TargetOpKind op, const TypeList& arg_types) const = 0;

    virtual bool valid_ctor_call(TypeId target, const TypeList& arg_types) const = 0;

    virtual bool can_implicit_cast(TypeId src_ty, TypeId dest_ty) const = 0;

private:
    const BuiltinFunction* get_builtin_fn(std::string_view fn_name) const;
    std::optional<BuiltinGlobal> get_builtin_global(std::string_view global_name) const;
};

} // namespace stc
