// example meant to demonstrate sandbox usage

// sandboxes can include and use libstc headers (exported and internal)
#include "api/stc.hpp"           // C++ API for the transpiler
#include "common/utils.hpp"      // ScopeGuard
#include "frontend/jl/utils.hpp" // jl::check_exception

// they can also access libjulia if needed
// (recommended to include via stc's wrapper guard in external/julia.hpp)
#include "external/julia.hpp"
// define this when including Julia since this is the main entry point of the sandbox program
JULIA_DEFINE_FAST_TLS

// fmt is also provided for easy logging and formatting
#include <fmt/format.h>

#include <cstdlib>
#include <iostream>
#include <string_view>

int main() {
    jl_init();

    // sandboxes are free to use libstc internals (like the ScopeGuard util)
    const stc::ScopeGuard jl_guard{[] { jl_atexit_hook(0); }};

    jl_eval_string("using JuliaGLM"); // JuliaGLM is required for transpilation

    // always check for Julia-side errors
    if (stc::jl::check_exceptions()) {
        std::cerr << "couldn't initialize Julia runtime (JuliaGLM might not be available in "
                     "current environment)\n";
        return EXIT_FAILURE;
    }

    jl_eval_string("println(\"print some text through Julia\")");
    // error check omitted for brevity

    // clang-format off
    // julia_code must represent a compound expression
    static constexpr std::string_view julia_code =
        "begin\n"
            "@gl_out global frag_col::Vec4\n"
            "function main()\n"
            "    global frag_col::Vec4\n"
            "    frag_col = Vec4(0, 1, 0, 1)\n"
            "end\n"
        "end\n";
    // clang-format on

    // see docs for using the transpiler's public C++ API
    stc::TranspilerConfig cfg{};
    cfg.dump_sema = true;

    // or the C API (pulled in transitively via stc.hpp, accessible under stc::api in C++, similar
    // to the main C++ API)
    fmt::println("libstc ABI version: {}", stc::api::stc_abi_version());

    stc::api::TranspilationResult result = stc::api::transpile(julia_code, cfg);

    if (result.status == stc::api::Status::Success)
        fmt::println("Transpiled GLSL code:\n{}", result.code);
    else
        fmt::println("Transpilation failed\n");

    return result.status == stc::api::Status::Success ? EXIT_SUCCESS : EXIT_FAILURE;
}