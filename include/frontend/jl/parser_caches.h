#pragma once

#include "julia_wrapper.h"

namespace stc::jl {

struct JLParserSymbolCache {
    jl_sym_t* block      = jl_symbol("block");
    jl_sym_t* global     = jl_symbol("global");
    jl_sym_t* local      = jl_symbol("local");
    jl_sym_t* eq         = jl_symbol("=");
    jl_sym_t* call       = jl_symbol("call");
    jl_sym_t* if_        = jl_symbol("if");
    jl_sym_t* elseif     = jl_symbol("elseif");
    jl_sym_t* return_    = jl_symbol("return");
    jl_sym_t* break_     = jl_symbol("break");
    jl_sym_t* continue_  = jl_symbol("continue");
    jl_sym_t* parameters = jl_symbol("parameters");
    jl_sym_t* double_col = jl_symbol("::");
    jl_sym_t* nothing    = jl_symbol("nothing");
    jl_sym_t* while_     = jl_symbol("while");
};

struct JLParserTypeCache {
    jl_datatype_t* uint128;

    explicit JLParserTypeCache() {
        uint128 =
            reinterpret_cast<jl_datatype_t*>(jl_get_global(jl_core_module, jl_symbol("UInt128")));

        assert(uint128 != nullptr &&
               "failed to load uint128 datatype from julia through the core module");
    }
};

} // namespace stc::jl
