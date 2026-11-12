#pragma once

#include "frontend/jl/rt/module_cache.hpp"
#include "frontend/jl/rt/sym_cache.hpp"
#include "frontend/jl/rt/type_cache.hpp"

namespace stc::jl::rt {

// ! IMPORTANT !
// this is a class intended to avoid repeated lookup for static parts of julia, so that they are
// only ever retrieved once through libjulia. it's intended use is for things like symbol caching,
// Base/Core function caching, module caching (in some scenarios), etc.
// it should never be used to cache things that are not guaranteed to be rooted by the julia GC for
// the lifetime of JuliaRTEnv
struct JuliaRTEnv {
    JuliaModuleCache module_cache;
    JuliaSymbolCache symbol_cache{};
    JuliaTypeCache type_cache;

    explicit JuliaRTEnv(std::string_view julia_glm_path)
        : module_cache{julia_glm_path}, type_cache{module_cache} {}
};

} // namespace stc::jl::rt
