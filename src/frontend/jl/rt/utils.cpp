#include "frontend/jl/rt/utils.h"

#include "external/tracy.h"

namespace stc::jl::rt {

jl_value_t* find_jl_function(std::string_view lookup_chain, rt::JuliaRTEnv& env,
                             JuliaModule& root_mod, bool throw_on_not_found) {
    size_t last_dot_idx = lookup_chain.find_last_of('.');

    // TODO: check for dot end/begin

    JuliaModule* mod = &root_mod;

    if (last_dot_idx != lookup_chain.npos) {
        if (throw_on_not_found) {
            mod =
                &env.module_cache.get_mod_or_throw(lookup_chain.substr(0, last_dot_idx), root_mod);
        } else {
            auto maybe_mod =
                env.module_cache.get_mod(lookup_chain.substr(0, last_dot_idx), root_mod);

            if (!maybe_mod.has_value())
                return nullptr;

            mod = &maybe_mod->get();
        }
    }

    assert(mod != nullptr);

    return mod->get_fn(lookup_chain.substr(last_dot_idx + 1), throw_on_not_found);
}

jl_value_t* parse_code(std::string_view julia_code, JuliaModuleCache& module_cache) {
    ZoneScoped;
    ZoneText(julia_code.data(), julia_code.size());

    jl_value_t* code_jl_str = nullptr;
    jl_value_t* parsed_ast  = nullptr;
    JL_GC_PUSH2(&code_jl_str, &parsed_ast); // NOLINT

    const ScopeGuard jl_gc_pop_guard{[]() { JL_GC_POP(); }};

    jl_value_t* parse_fn = module_cache.meta_mod.get_fn("parse", false);
    if (parse_fn == nullptr)
        return nullptr;

    // implemented as a simple memcpy in libjulia, avoids strlen (==> null termination agnostic)
    code_jl_str = jl_pchar_to_string(julia_code.data(), julia_code.size());
    if (check_exceptions())
        return nullptr;

    parsed_ast = jl_call1(parse_fn, code_jl_str);
    if (check_exceptions())
        return nullptr;

    return parsed_ast;
}

} // namespace stc::jl::rt
