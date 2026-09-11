// example meant to demonstrate sandbox usage

// sandboxes can include and use libstc headers
#include "api/transpiler.h"
#include "common/utils.h"      // ScopeGuard
#include "frontend/jl/utils.h" // jl::check_exception

// they can also access libjulia if needed
// (recommended to include via the libstc guard)
#include "external/julia.h"
// define this when including Julia since this is the main entry point of the sandbox program
JULIA_DEFINE_FAST_TLS

// fmt is also provided for easy logging and formatting
#include <fmt/format.h>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

int main() {
    fmt::println("libstc ABI version: {}", stc::api::stc_abi_version());

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
    static constexpr std::string_view julia_code =
        "begin\n"
            "@gl_out global frag_col::Vec4\n"
            "function main()\n"
            "    global frag_col::Vec4\n"
            "    frag_col = Vec4(0, 1, 0, 1)\n"
            "end\n"
        "end\n";
    // clang-format on

    // see docs for using the transpiler's public C++ (or C) API
    stc::TranspilerConfig cfg{};
    cfg.dump_sema = true;

    std::optional<std::string> glsl_code = stc::api::transpile<true>(julia_code, std::nullopt, cfg);

    if (glsl_code)
        fmt::println("Transpiled GLSL code:\n{}", *glsl_code);
    else
        fmt::println("Transpilation failed\n");

    return glsl_code ? EXIT_SUCCESS : EXIT_FAILURE;
}