#pragma once

#include <optional>
#include <string_view>

#include "frontend/jl/rt/env.hpp"

#include "external/julia.hpp"

namespace stc::jl::rt {

// lookup chain's expected format is: [ModuleA.ModuleB.(...).]fn_name
// the chain is expected to start from root_mod
[[nodiscard]]
jl_value_t* find_jl_function(std::string_view lookup_chain, rt::JuliaRTEnv& env,
                             JuliaModule& root_mod, bool throw_on_not_found = true);

[[nodiscard]]
inline jl_value_t* find_jl_function(std::string_view lookup_chain, rt::JuliaRTEnv& env,
                                    bool throw_on_not_found = true) {
    return find_jl_function(lookup_chain, env, env.module_cache.main_mod, throw_on_not_found);
}

// these were mainly added to satisfy clang-tidy's no implicit int -> bool cast rule
// without having to disable it globally, or mark it at every check to these

[[nodiscard]]
inline bool is_type(jl_value_t* value) {
    return jl_is_type(value) != 0;
}

[[nodiscard]]
inline bool is_concrete_type(jl_value_t* value) {
    return jl_is_concrete_type(value) != 0;
}

[[nodiscard]]
inline bool is_array_type(jl_datatype_t* value) {
    return jl_is_array_type(value) != 0;
}

// tries to parse a Julia code snippet string into a Julia AST (Expr-format)
// if an error occurs Julia-side, the error is printed and nullptr is returned
// result is jl_value_t*, since it might be a literal, LineNumberNode, Expr, etc.
[[nodiscard]]
jl_value_t* parse_code(std::string_view julia_code, JuliaModuleCache& module_cache);

} // namespace stc::jl::rt
