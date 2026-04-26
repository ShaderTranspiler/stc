// ! ONLY EVER INCLUDE <julia.h> THROUGH THIS WRAPPER !
// Reasoning:
// MSVC fails to compile without NOMINMAX because of stuff like std::numeric_limits<T>::max()
// getting recognized as a macro invocation. However, mingw gcc (for example) already defines
// NOMINMAX internally, so it will start spitting out macro redef warnings for NOMINMAX, if it's
// redefined here.

#ifndef NOMINMAX
#define NOMINMAX
#define STC_DEFINED_NOMINMAX
#endif

// sanity check #1
#ifdef max
static_assert(false, "max macro defined pre julia.h include");
#endif

#ifdef min
static_assert(false, "min macro defined pre julia.h include");
#endif

// string needs to be included before julia to fix some very specific issues under some very
// specific configurations
#include <string>

#include <julia.h>

// undef NOMINMAX so that if anything else tries to define min/max, it'll be noticed and handled
// separately, with proper context on that other include
// or, for systems that define it externally, leave it as is
#ifdef STC_DEFINED_NOMINMAX
#undef NOMINMAX
#undef STC_DEFINED_NOMINMAX
#endif

// sanity check #2
#ifdef max
static_assert(false, "max macro defined post julia.h include");
#endif

#ifdef min
static_assert(false, "min macro defined post julia.h include");
#endif

#if JULIA_VERSION_MAJOR == 1 && JULIA_VERSION_MINOR <= 10 && !defined(jl_unwrap_unionall)

static inline jl_value_t* stc_unwrap_unionall(jl_value_t* v) {
    while (jl_is_unionall(v)) {
        v = ((jl_unionall_t*)v)->body;
    }
    return v;
}

#define jl_unwrap_unionall(v) stc_unwrap_unionall(v)

#endif

#ifndef jl_is_linenumbernode
#define jl_is_linenumbernode(v) jl_typetagis(v, jl_linenumbernode_type)
#endif
