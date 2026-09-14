# Shader Transpiler Core (STC)

[![CI](https://github.com/ShaderTranspiler/stc/actions/workflows/ci.yml/badge.svg)](https://github.com/ShaderTranspiler/stc/actions/workflows/ci.yml)

**NOTE:** This project is being developed as my CS BSc thesis at ELTE.

_Shader Transpiler Core_ (STC) is a Julia to GLSL transpiler that aims to help GPU-side parallelization of regular, generic Julia code. The goal is to be able to take code snippets written as regular, CPU-side Julia code and be able to run them on the GPU. The motivation came from the [Juliagebra](https://github.com/Csabix/Juliagebra) project, where this allowed the implementation of a faster tessellation model for parametric curves and surfaces. These geometries have functions that come from the end user, and thus cannot be previously written up as regular OpenGL shaders. By using STC, Juliagebra can use transpiled compute shaders for GPU-parallelized tessellation, without requiring the end user to write any GPU code. This use case also fuelled the high performance nature of the transpiler (achieving 0.5-2 ms transpilations during typical usage), as a fast transpilation time means only a small time cost is added to Juliagebra's geometry initialization pipeline.

As the name suggests, STC is planned from the beginning as a tool that's capable of general shader transpilation, and isn't architecturally bound to Julia and GLSL. Several design decisions were made to facilitate easy addition of new backend targets later on. New source languages are also possible under the current architecture, though that would require considerably more effort. See later sections on internals to see how this decoupling is done, what might need to be refined before a new backend can be added and what already fully supports augmentation.

The transpiler can be used both as a dynamically linked library and as a CLI tool, though the latter is mainly useful for testing, benchmarking and development. The former allows easy integration into codebases for automatic shader generation, but unless working with C/C++, it's not recommended to use on its own (thus the _Core_ in the project name).

An actively generated JLL wrapper for the library can be found in the [stc_jll](https://github.com/ShaderTranspiler/stc_jll.jl) repository, which allows easy installation via Julia's Pkg interface and plug and play usage through `ccall`s.

A more high level Julia abstraction is available in the [ShaderTranspiler.jl](https://github.com/ShaderTranspiler/ShaderTranspiler.jl) Pkg (not hosted in the general Julia package registry), which wraps low-level abstractions behind high-level Julia constructs. This is the recommended way to use the transpiler from Julia, and relies on `stc_jll` internally to interact with the library. Unless manual building is desirable, it's also easiest to acquire the latest CLI through this package, which it exposes as a Pkg App (requires Julia 1.12).

# Table of Contents

- [Shader Transpiler Core (STC)](#shader-transpiler-core-stc)
- [Table of Contents](#table-of-contents)
- [Requirements](#requirements)
- [Getting Started](#getting-started)
  - [Transpilation Example](#transpilation-example)
  - [CLI](#cli)
  - [Library](#library)
  - [Julia Pkg](#julia-pkg)
- [Building the Project](#building-the-project)
  - [Build Output](#build-output)
  - [Linking libjulia](#linking-libjulia)
  - [Compilation Flags](#compilation-flags)
  - [Note for Development Using Visual Studio](#note-for-development-using-visual-studio)
  - [Sandbox](#sandbox)
  - [ccache (optional)](#ccache-optional)
  - [clang-format (optional)](#clang-format-optional)
  - [clang-tidy (optional)](#clang-tidy-optional)
  - [Catch2 (optional)](#catch2-optional)
  - [Doxygen (with Graphviz) (optional)](#doxygen-with-graphviz-optional)
  - [Tracy (optional)](#tracy-optional)
  - [CMake Options](#cmake-options)
- [Project Structure](#project-structure)
- [Transpiler Architecture](#transpiler-architecture)
  - [Transpilation Pipeline](#transpilation-pipeline)
    - [Parser](#parser)
    - [Semantic Analysis / Symbol Resolution](#semantic-analysis--symbol-resolution)
    - [Lowering](#lowering)
    - [Code Generation](#code-generation)
  - [Extending With New Target Languages](#extending-with-new-target-languages)
- [Software Architecture](#software-architecture)
  - [Arena Allocation](#arena-allocation)
    - [Lightweight AST nodes](#lightweight-ast-nodes)
  - [Language-agnostic Type Descriptors](#language-agnostic-type-descriptors)
  - [LLVM-style RTTI](#llvm-style-rtti)
    - [Replacing `dynamic_cast`](#replacing-dynamic_cast)
    - [Static Polymorphism via CRTP](#static-polymorphism-via-crtp)
    - [X-Macro System](#x-macro-system)
  - [Partial Move Semantics](#partial-move-semantics)

# Requirements

Building the transpiler requires:
- **CMake 3.16** or newer
- a **C++20** compiler (GCC is recommended, clang and MSVC are also supported)
- a **v1.11** or newer **Julia** installation

Everything else is optional and automatically fetched by CMake when needed, or only enabled if available (see [Building the Project](#building-the-project)).

Note that transpilation currently requires [JuliaGLM](https://github.com/Csabix/JuliaGLM) to be available in the Julia environment, since the shader types and builtins the transpiler refers to are all defined there.
This requirement is completely independent from building, JuliaGLM is needed during the runtime of the transpiler.

# Getting Started

There are three ways to use the transpiler. The first subsection introduces an example code snippet, with subsequent subsections demonstrating how to actually run the transpiler using the three different methods.

## Transpilation Example

Let's take the following Julia shader. It produces a fragment shader of a color gradient with a repeating wave of shrinking ellipses. The Julia shader can be transpiled, compiled and viewed in one click with the [ShaderSandbox.jl](https://github.com/ShaderTranspiler/ShaderSandbox.jl) app.

```julia
@gl_uniform global time::Float32
@gl_uniform global resolution::IVec2
@gl_out     global frag_col::Vec4

function main()
    global frag_col

    uv = gl_FragCoord[:xy] / resolution[:xy]
    d = length(uv - 0.5)
    c = 0.5 + 0.5 * cos(time + d * 20.0)

    frag_col = Vec4(c * uv[:x], c * uv[:y], c, 1.0)
end
```

It's worth pointing out that the only GLSL-specific code the user writes is in the form of type qualifier decorators (e.g. `@gl_uniform`) and the global builtin variables (e.g. `gl_FragCoord`).

The code also follows Julia semantics: `frag_col` has to be explicitly marked `global` inside the function too, otherwise the binding would have to be treated as a new local definition in the last line. These get stripped in the generated code, but the main philosophy of the transpiler is to generate code that matches the semantics of the original snippet instead of trying to guess the developer's intention (except for a couple of toggleable, deliberate convenience shortcuts).

Running the above code through the transpiler produces:
```glsl
#version 460

uniform float time;
uniform ivec2 resolution;
out vec4 frag_col;

void main()
{
    vec2 uv_stc_usym_2 = (gl_FragCoord.xy) / (resolution.xy);
    float d_stc_usym_1 = length((uv_stc_usym_2) - (0.500000000f));
    float c_stc_usym_0 = (0.500000000f) + ((0.500000000f) * (cos((time) + ((d_stc_usym_1) * (20.000000000f)))));
    frag_col = vec4((c_stc_usym_0) * (uv_stc_usym_2.x), (c_stc_usym_0) * (uv_stc_usym_2.y), c_stc_usym_0, 1.000000000f);
    return ;
}
```

As it is readily apparent, the generated code is not meant for human reading, it's meant to be used as a middle step in automated pipelines.

## CLI

The CLI is the quickest way to get up and running with the transpiler. It can be used to perform single-file transpilation, benchmarking and debugging. It's mainly useful during development to track the internal state of the transpiler between passes and for quick experimentation.

It can be built as a target separate from the core library (enabled by default, see [Building the Project](#building-the-project)), or it can be installed easily via Julia as a Pkg App. Note that using Pkg Apps requires Julia v1.12 or newer.

The easy way to get the CLI is through the Pkg App method, which is distributed at [ShaderTranspiler.jl](https://github.com/ShaderTranspiler/ShaderTranspiler.jl). It can be installed using the Julia REPL:
```bash
$ julia
julia> ]
pkg> app add https://github.com/ShaderTranspiler/ShaderTranspiler.jl
```

This installs a Julia script wrapping the CLI under `~/.julia/bin`. Adding it to `PATH` enables it to be used like a regular executable. Because the built CLI executable is invoked via Julia, performance may be slightly worse than when building and running the native CLI executable.

Building manually produces the CLI to `build/bin/stc`, as a native executable.

Some noteworthy CLI options:
- `-o <path>`: write the generated code into a file at `<path>` (only prints to `stdout` by default)
- `--gl-version <ver>`: set the version number used in the `#version` directive (defaults to 460). Note that this only affects the `#version` directive, transpilation always happens according to the 460 language rules.
- `--err-dump partial`: on error, dump the problematic AST subtree
- `--dump-parsed`, `--dump-sema`, `--dump-lowered`: prints the intermediate state of the AST, after the given pass
- `--track-bindings`: prints step-by-step reasoning for how each symbol's scope binding was determined
- `--it <n>`: runs the transpilation `n` times repeatedly (for benchmarking)
- `--no-coerce-f32`, `--no-coerce-i32`: turn off forcing Float64 and Int64 literals to their 32-bit counterparts

See the output of `stc --help` for an exhaustive list of transpiler options.

## Library

The produced library (`libstc`) exposes both a C++ and a C API (the latter serving as the basis for the Julia bindings). Both live under `include/api/` and the `stc::api` namespace. The two APIs have proper documentation, available [here](https://shadertranspiler.github.io/stc/).

A simple illustration for the C++ API:
```cpp
#include <external/julia.h> // guarded Julia include
#include <api/transpiler.h> // transpiler API
// ... (iostream, string_view, optional, cstdlib)

static constexpr std::string_view julia_code = "...";

int main() {
    jl_init(); // init a Julia context for the transpiler
    jl_eval_string("using JuliaGLM"); // JuliaGLM is required for transpilation

    stc::TranspilerConfig cfg{};
    cfg.use_tabs = true; // supports mostly the same options as the CLI

    // benchmarking is toggled with the template argument
    // the first argument is the code as a string, or as a jl_value_t* pointing to an Expr in Julia memory
    // the second option controls the file path for logging
    // last option is the config to use
    std::optional<std::string> glsl = stc::api::transpile<false>(julia_code, "path/of/source/file", cfg);

    if (!glsl) // transpilation failed
        return EXIT_FAILURE;

    std::cout << *glsl;

    jl_atexit_hook(0); // clean up Julia safely
    return EXIT_SUCCESS;
}
```

The same thing using the C API:
```c
#include <external/julia.h>
#include <api/transpiler.h>
// ... (stdio.h, stdlib.h, stdbool.h)

static const char* julia_code = "...";

#define EXPECTED_STC_ABI_VERSION 0

int main() {
    if (stc_abi_version() != EXPECTED_STC_ABI_VERSION)
        return EXIT_FAILURE;

    jl_init();
    jl_eval_string("using JuliaGLM");

    void* cfg_handle = stc_create_cfg();
    if (cfg_handle == NULL) // failed config creation (unlikely)
        return EXIT_FAILURE;

    stc_set_use_tabs(cfg_handle, true);

    void* result_handle = stc_transpile_code(julia_code, false, cfg_handle);
    
    // internal transpilation errors are reported as a NULL result
    if (result_handle == NULL) {
        stc_free_cfg(cfg_handle);
        return EXIT_FAILURE;
    }

    const char* glsl_code = stc_get_result(result_handle);
    
    // user failures are reported as empty strings
    // (detailed errors are printed to stderr during the transpilation)
    if (glsl_code[0] == '\0') {
        stc_free_cfg(cfg_handle);
        return EXIT_FAILURE;
    }
    
    printf("%s\n", glsl_code);

    stc_free_result(result_handle);
    stc_free_cfg(cfg_handle);

    return EXIT_SUCCESS;
}
```

The C and C++ APIs follow roughly the same design, the main difference is their memory ownership model. The C API returns internal handles through which later queries and modifications can be made to the given resource, and requires the developer to free them explicitly at the end.

The functions `stc::api::transpile` and `stc_transpile` accept either a string input for the raw code, or a `jl_value_t*` input for the Julia-parsed AST. The latter is faster if the AST is already accessible, since the transpiler can begin the pipeline immediately, while the former option needs to invoke the Julia parser first.

A few important notes:
- Initializing the Julia runtime is the caller's responsibility. This is because in most use cases a Julia runtime should already be accessible to the host application.
- It was stripped from the snippets for brevity, but if you're initializing the Julia runtime from C/C++, the `JULIA_DEFINE_FAST_TLS` macro should be defined (exactly once). It's also recommended to include `external/julia.h` instead of `julia.h` directly, since it acts as a wrapper safeguard for inclusion.
- Linking requires both `libstc` and `libjulia`.

Take a look at `sandbox/sb_example_sandbox.cpp` for a slightly more robust and documented example.

## Julia Pkg

For Julia users, the [ShaderTranspiler.jl](https://github.com/ShaderTranspiler/ShaderTranspiler.jl) package is the recommended inclusion method. It handles pulling in the correct version of the library and abstracts away C's lower level memory model and type system behind high-level Julia types. It also provides access directly to the C API without manual `ccall`s, so it's recommended even for C-style usage.

Since none of [JuliaGLM](https://github.com/Csabix/JuliaGLM), [stc_jll](https://github.com/ShaderTranspiler/stc_jll.jl) (hosting the prebuilt binaries for Julia access) and [ShaderTranspiler.jl](https://github.com/ShaderTranspiler/ShaderTranspiler.jl) are available in the general registry, they have to be manually added through their GitHub repositories:
```bash
$ julia
julia> ]
pkg> add https://github.com/Csabix/JuliaGLM
pkg> add https://github.com/ShaderTranspiler/stc_jll.jl
pkg> add https://github.com/ShaderTranspiler/ShaderTranspiler.jl
```

They can be tagged with their repository's URL under the `[sources]` section of `Project.toml` to avoid having to specify the URLs explicitly for new installations of depending projects and packages.

If you're not working on the transpiler itself, this is the recommended way to bundle the transpiler.

# Building the Project

The project uses CMake for the build and development pipeline. It pulls together a couple of different tools and build requirements. Entries marked _(optional)_ will not cause the config/build process to fail if they are not available on the development system, but they will be automatically enabled when accessible via `PATH` (and not explicitly disabled through options).

The standard build flow is the usual:

```bash
cmake -B build
cmake --build build
```

The top level `CMakeLists.txt` handles options, targets and installation. The self-contained parts live in `cmake/`:

- `find_julia.cmake`: locates the Julia installation from which libjulia is linked, and exposes it as an `STC::Julia` target
- `dev_tools.cmake`: ccache, clang-format and clang-tidy setup
- `docs.cmake`: Doxygen and Graphviz setup
- `gen_build_meta.cmake`: runs as a script at build-time to regenerate build metadata headers

## Build Output

A build produces the following in the build directory:

- `lib/`: the shared library (`libstc`), and its import library where applicable
- `bin/`: the CLI executable, named `stc`
- `bin/sandbox/`: sandbox executables, when sandboxing is enabled
- `test/`: the `unit_tests` executable, when testing is enabled
- `docs/`: the docs in HTML (`docs/html/`), LaTeX (`docs/latex/`) and/or PDF (`docs/stc_{version}_docs.pdf`) formats, when the given docs format is enabled.

The library is built in two steps: an object library (`stc_objs`) holds the compiled translation units, and the shared library is assembled from those objects. This exists so the test target can link the objects directly and reach internal symbols that are not part of the API, and are as such hidden in the final shared library.

## Linking libjulia

The CMake configuration uses the julia executable (from `PATH`) to locate the libjulia shared library, its headers and the Julia version being built against, and exposes all of it through an imported `STC::Julia` target. This should work regardless of development platform, although it has only been tested in Windows and Linux environments. Note that this means building the transpiler requires an installed Julia executable, or at least a locally available version of libjulia, with all the Julia-specific options explicitly specified (see below). In the future there might be a Julia-independent option to build the transpiler itself without the Julia frontend, but as Julia is currently the only supported source language, there is no use for this right now.

To build against an install the julia executable doesn't point to, pass `JULIA_INCLUDE_DIR`, `JULIA_LIB_DIR`, `JULIA_BIN_DIR` and `STC_JULIA_VERSION` explicitly. These have to be given all at once or not at all. Mixing an explicit path with executable-resolved ones is rejected, because it can otherwise silently produce a build that compiles against one Julia's headers and links another's library.

Linking libjulia unfortunately means a lot of static and runtime analysis tools start reporting violations from libjulia and LLVM. One suppression file is provided for LSan in `misc/lsan.supp`, and one for Valgrind in `misc/valgrind.supp`. Valgrind still detects a couple of "still reachable"s, but their frame info is so generic that I couldn't find a way to suppress them with a general pattern.

For convenience, the script `scripts/init_debug_env.sh` can be used to locate (or specify) the suppression files and add them to the current environment. For this, the script must be run sourced.

## Compilation Flags

On MSVC:

- `/W4`: strict, "linter-like" compile-time warning behavior
- `/GR-`: RTTI off, the AST uses its own kind-based `isa`/`dyn_cast` instead
- `/Zc:preprocessor`: the standard-conforming preprocessor is required by the X-macro system (proper `__VA_ARGS__` expansion specifically)

Elsewhere:

- `-Wall -Wextra -Wpedantic -Wconversion -Wreorder`: strict warnings, like on MSVC
- `-fno-rtti`: same reasoning as `/GR-`
- `-fvisibility=hidden`: only `STC_API`-marked symbols are exported

Link time optimization is enabled automatically for Release and MinSizeRel, if the toolchain supports it.

Sanitizers are opt-in through `STC_USE_SAN`, and only apply to Debug configurations. On MSVC that means `/fsanitize=address` and `/Zi`, with incompatible options disabled. Elsewhere it means `-fsanitize=address,undefined -fno-omit-frame-pointer -g`.

Note that sanitizers and libjulia do not play nicely. Julia loads some of its dependencies with `RTLD_DEEPBIND`, which is incompatible with the sanitizer runtime, so anything that initializes the Julia runtime refuses to start. The option is still useful for sandboxes while working on internals completely unrelated to libjulia (which is most of the codebase).

## Note for Development Using Visual Studio

The recommended way to use Visual Studio for development is to simply not, whenever possible. VS doesn't naturally support a lot of tools (e.g. clang-tidy, Catch2) that other environments can detect and use without extra setup required.

If that isn't applicable, the second-best way is to open the root directory directly, rather than the CMake generated solution and project files. VS will still integrate with CMake and use it for configuring and building. This allows the use of tools like clang-tidy, which Visual Studio (and MSVC in general) mostly ignores otherwise, when ran directly on the generated files.

There are a couple of build configurations provided (see _CMakeSettings.json_ for details). The main difference is the generator they use (Ninja or VS, where Ninja is **highly** recommended over VS), and whether Debug or Release is used. All configs are x64-based and use MSVC, though both can be extended later if needed.

VS support is not perfect, but it should be a viable option for development. As I personally don't mainly use VS, I did not want to dedicate any more time to implementing every single build step twice. One imperfection is that with Ninja as a generator, clang-tidy correctly respects the warnings as errors option during building, whereas with the VS generators, it does not. Another nuisance is that VS tends to miss and/or misrecognize the more dynamic parts of the build process (e.g. addition/removal of sandbox targets), this is usually solved by deleting the CMake cache and reconfiguring. If that doesn't fix it, restarting VS might help.

I also don't use MSVC as my main compiler, so some commits may break VS/MSVC-compatibility, but these are usually fixed sooner or later, when I notice them. `scripts/build_all.ps1` configures and builds gcc, clang and MSVC in Debug and Release in one go, which is the usual way I catch those.

## Sandbox

Any source files named `sb_*.cpp` in the `sandbox/` directory will be built as a separate executable with a unique target. These all have the built library linked automatically, and its produced output copied next to the executables.

Through sandbox executables, it's easy to produce example, demo, manual test or playground code that interacts with the library from an "external" perspective. Most files in `sandbox/` will thus probably contain development-related, temporary code that is useful while writing the transpiler. `sb_example_sandbox.cpp` is kept around as a demonstration of what a sandbox can reach and how.

## [ccache](https://ccache.dev/) (optional)

ccache is included in the build process to speed up compilation time through compiler caching.

## [clang-format](https://clang.llvm.org/docs/ClangFormat.html) (optional)

clang-format is used for consistent code style. The CMake config provides the `check_format` and `fix_format` targets for retrieving a list of code style violations and automatically fixing them, respectively. The rules set up in the `.clang-format` file are mostly out of personal preference, that is, what I find to be _"readable"_ and _"nice-looking"_ for C++ code.

A sample pre-commit hook script is also included in `scripts/`, which verifies that the code contains no style violations. The script validates the current state of the directory, so any changes that are present but not staged should be stashed before creating the commit.

```bash
cp scripts/pre-commit .git/hooks/
```

## [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) (optional)

clang-tidy is used for static analysis. A strict set of rules is defined in the `.clang-tidy` file, to keep the code modern, readable and safe. Since most of this project lives in headers, the header filter includes everything and then excludes fetched dependencies, so new header directories are never accidentally left unchecked.

## [Catch2](https://github.com/catchorg/Catch2) (optional)

Catch2 is used for unit tests. It is acquired through FetchContent and built along with the library. Tests can be run with `ctest` from the build directory, by building the `test` target, or by building and running the `unit_tests` executable directly.

Testing is off by default. Enable it with `-DSTC_BUILD_TESTS=ON`. The standard `BUILD_TESTING` variable is honored as a fallback, so pipelines that prefer it keep working.

## [Doxygen](https://doxygen.nl) (with [Graphviz](https://graphviz.org)) (optional)

Doxygen is used for automatic documentation generation, in HTML and LaTeX. Documentation can be generated through the `docs` target. If `pdflatex` is found, `docs_pdf` additionally compiles the LaTeX output to PDF and copies the result next to the HTML docs.

The documentation parameters are currently experimental. I'm trying to find the balance between a version that only produces meaningful info for things with proper individual comment docs and a version that outputs every little meaningless detail and blows docs up to 900 pages.

Most of the public API is annotated by now, so the generated docs are more useful than they used to be, but coverage of the internals is still non-existent.

## [Tracy](https://github.com/wolfpld/tracy) (optional)

Tracy is used for profiling. It is fetched through FetchContent and linked into the library, CLI and tests when `-DSTC_ENABLE_PROFILING=ON` is passed, which also defines the `STC_PROFILING` macro so the instrumentation in the code becomes active.

## CMake Options

Options are documented in `Name (Default): Description` format.

- `STC_BUILD_CLI` (`ON`): enables the CLI target (disabling saves a bit of time when it's enough to build the lib)
- `STC_BUILD_DOCS` (`OFF`): enables the docs-related targets
- `STC_BUILD_TESTS` (`OFF`, or the value of `BUILD_TESTING` if that is set): enables the unit test targets. Catch2 is only fetched if this is enabled.
- `STC_BUILD_SANDBOX` (`OFF`): enables the sandbox targets
- `STC_ENABLE_PROFILING` (`OFF`): fetches and links Tracy, and defines `STC_PROFILING`
- `STC_USE_CCACHE` (`ON`): uses ccache for compiler caching, if installed locally
- `STC_USE_FORMAT` (`ON`): enables the formatting targets, if clang-format is installed locally
- `STC_USE_TIDY` (`OFF`): enables static analysis through clang-tidy, if installed locally
- `STC_USE_SAN` (`OFF`): enables ASan and UBSan for Debug builds (see the libjulia warning above)
- `STC_OFFICIAL_BUILD` (`OFF`): marks the builds produced under this configuration as official (this isn't a validation/verification feature, it's just a flag to easily distinguish between local development builds and official releases for debugging, mostly used by CI).

Not options as such, but respected:

- `JULIA_INCLUDE_DIR`, `JULIA_LIB_DIR`, `JULIA_BIN_DIR`, `STC_JULIA_VERSION`: bypass Julia resolution, all four need to be passed together (see [Linking libjulia](#linking-libjulia))
- `STC_COMMIT_HASH`, `STC_BUILD_TIMESTAMP`: pin build metadata instead of querying git and the system clock, which is what the packaging scripts use for reproducible builds. Pin `STC_BUILD_TIMESTAMP` if you're frequently rebuilding during development (build timestamp immediately invalidates the library's build target and every executable target transitively).

# Project Structure

- `include/` and `src/` -- the transpiler core itself. The two mirror each other: src/x/y/z.cpp implements the header at include/x/y/z.h, and most directories correspond to a namespace. Most of the codebase is header-heavy thanks to extensive use of templates, so some directories may exist only under `include/`.
  - `api/` (`stc::api`) -- the public API of the library. The C ABI used by the Julia wrapper and the C++ entry points that orchestrate the passes into a pipeline.
  - `frontend/jl/` (`stc::jl`) -- the Julia frontend. The AST, the parser, sema, symbol resolution and lowering passes.
    - `frontend/jl/rt/` (`stc::jl::rt`) -- abstractions over the libjulia runtime interface. Module, symbol and type caches, and safe query functions over the embedded-Julia environment.
  - `sir/` (`stc::sir`) -- the shader AST. Target-independent, and the middle layer any future backend attaches to.
  - `backend/glsl/` (`stc::glsl`) -- GLSL code generation, plus the `TargetInfo` implementation and builtin tables that earlier passes query.
  - `types/` (`stc::types`) -- the type system. Type descriptors, the interning pool and qualifiers. Shared by every language (see [Language-agnostic Type Descriptors](#language-agnostic-type-descriptors)).
  - `ast/` (`stc`) -- Base AST infrastructure that isn't specific to either language. The context base, the CRTP visitor base and the symbol pool all live here.
  - `common/` (mainly `stc`) -- general-purpose code used throughout the library. The bump arena, source location tracking, the target info interface, config and a set of general and not-so-general utilities.
  - `external/` -- wrappers around third-party headers that need care at the include site (libjulia, Tracy). Header-only.
  - `meta/` -- templates for build and configure time metadata headers, filled in by CMake. Header-only.
  - Node kinds, qualifiers and GLSL builtins are declared in `.def` files under `*_defs/` directories, expanded through the X-macro helpers in `common/x_macros/` (see [X-Macro System](#x-macro-system)). They are data, not code.
- `cli/` -- the command line program. Minimal, and the main (in-repo) consumer of the library API.
- `test/` -- Catch2 unit tests.
- `examples/` -- Transpilable Julia shaders, used both as usage examples and as transpilation tests.
- `sandbox/` -- scratch executables for manual testing against the built library (see `sandbox/sb_example_sandbox.cpp` for usage).
- `cmake/` -- build system modules. Julia resolution, dev tooling, docs and metadata generation.
- `scripts/` -- developer helper scripts: auto builder for `PowerShell`, debug environment setup and a recommended pre-commit hook.
- `misc/` -- suppression files for the analysis tools that libjulia upsets.
- `.github/workflows/` -- CI, docs deployment and release automation.

# Transpiler Architecture

The transpilation process is designed as a multi-pass pipeline, with a language-agnostic middle layer to decouple the frontend and backend layers. This allows new target languages to be added in a modular fashion, without the need to modify existing code.

Each pass traverses the tree representation produced by the previous pass (the initial one receiving the raw AST straight from Julia as its input), and builds a new tree that is passed onwards (with the exception of the final pass, which outputs the resulting code).

## Transpilation Pipeline

The current Julia -> GLSL pipeline consists of the following steps:

1. Parser
1. Semantic analysis (interleaved with a symbol resolution subpass)
1. Lowering
1. Code generation

### Parser

The first stage takes the AST coming from the Julia runtime directly (as an `Expr`) and creates a custom Julia AST representation from it, that fits into the transpiler's resource systems (see [Software Architecture](#software-architecture)).

The pass is performed on top of Julia's own parsing for two reasons: it integrates the input AST into the transpiler's architecture immediately and makes later processing easier. Julia's AST is flexible and dynamic, which is useful for its metaprogramming capabilities, but makes analysis hard to perform. A node's kind doesn't appear in the type system at all, it's stored as a head value inside an `Expr` (which is what almost every non-leaf node of the Julia AST is represented as).
This makes analysis cumbersome and cluttered with repeated equality checks to match certain cases instead of having the ability to rely on the type system, function overloading, etc.

It makes heavy use of the embedded libjulia features, and uses its getter and query functions instead of relying on libjulia's (undocumented) ABI to reduce breaking changes with new Julia versions.

### Semantic Analysis / Symbol Resolution

The goal of semantic analysis is to perform type checking and inference on the input AST. This is where the Julia subset restrictions are enforced. Dynamic typing, or non-transpilable Julia functionality is rejected, with detailed error messages that contain the exact source location (and optionally the problematic subtree in the AST). The visitor is based on the bidirectional type checking model, which, among other benefits, will allow the easy addition of Julia's subtyping later.

Due to Julia's liberal binding system, it's not possible to resolve every symbol in a single traversal pass. Later code can influence what scope a given symbol in earlier code points to. Instead of doing fix-point iteration, an interleaved symbol resolution pass is executed alongside semantic analysis. This pre-traverses every scope sema enters, and exists solely to determine what scope each symbol should bind to, based on its usage in the given scope. It checks for explicit scope declarations, whether it's ever assigned or just accessed, and based on these, assembles the binding table once it has looked at every AST node in the given scope.

It also resolves casting mismatches between Julia and the target language via the `TargetInfo` interface, and introduces explicit casts wherever they're needed to maintain the source language's semantics. Every interaction with the target language is abstracted to some extent; GLSL-specific assumptions and logic do not appear here. This is what allows the later addition of new target languages without a separate sema pass.

It also perform name mangling as its final step. Every identifier used in the code becomes globally unique, which ensures that differing scoping rules won't introduce variable shadowing differences in the generated code, or redefine symbols that are still in scope.

Sema is certainly the beefiest and most complex part of the entire codebase that handles almost all semantic analysis needed for producing the resulting code. Lowering expects this pass to already resolve any non-trivial transformations needed to be able to lower Julia to the language-agnostic shader AST.

### Lowering

Based on an AST decorated with type and binding information, it translates the Julia AST to the custom shader AST format. It doesn't perform any meaningful semantic analysis, it expects the AST to be already transformed to an easily resolvable state by sema.

As mentioned earlier, currently the shader AST favors GLSL somewhat, due to the deadline of my thesis. One of the reasons adding a new backend would be beneficial is that the shader AST could be generalized to a much more language-independent state.
That would make lowering act more as a bridge step rather than a simple translate-or-error pass.

### Code Generation

Code generation is a simple visitor pass that turns the general concepts in the shader AST into its corresponding GLSL construct. This is probably the simplest pass in the pipeline, as it does little more than read the shader AST and emit some string into a buffer based on that.

## Extending With New Target Languages

The main advantage of decoupling the frontend (Julia) from the current sole backend (GLSL) is the opportunity to add new targets without modifying the Julia-specific parts.

This is achieved by two architectural decisions. First, the Julia AST is lowered to a custom, language-agnostic shader AST, before any GLSL-specific logic runs. New backends can use this representation as a starting point. Second, Julia sema uses an abstract `TargetInfo` interface for backend-specific queries (accessing global constants and their types, resolving casting rules, etc.), which means it can be configured by implementing `TargetInfo` for the new language instead of modifying Julia-side sema code.

It's important to note that my thesis focuses on Julia to GLSL transpilation specifically. This, to adhere to the time limitations, resulted in some GLSL-specific concepts leaking into the sema logic itself (like the specific type qualifier decorators that are available, or sema always treating explicit casts as a constructor call, as it is done in GLSL). Adding a second target language will require some further abstraction to be introduced, but a good chunk of the work was already done during the initial development. All abstractions, systems and utilities were developed with this in mind, meaning they're ready to be extended with new languages.

# Software Architecture

The project is written in C++20, directly depending on `libjulia` (see [Linking libjulia](#linking-libjulia)) and `fmt` alone (some compilers used by `BinaryBuilder` for distribution don't support `std::format` properly).

The transpiler often uses certain design patterns and language constructs for abstraction repeatedly. Some of these serve performance purposes, others improve DevX, while some do both. The following section aims to collect and summarize the most common and fundamental architectural decisions used throughout the codebase.

A lot of these patterns were inspired by LLVM, though they have their own, custom implementation. This allowed some optimizations specific to this use case, as well as the placement of extra safety measures via modern C++ features (e.g. heavy use of concepts and `requires` constraints throughout the codebase).
For classes and functions implementing certain non-trivial patterns, one can usually find a reference article or code snippet in the comments.

## Arena Allocation

Most resources used during transpilation are allocated through a memory arena. This includes AST nodes, type descriptors and qualifiers, symbols and source location information. Each of these constructs has its own custom pool type that abstracts the arena usage behind high-level utilities specific to the given use case. For example, this is where type descriptors and symbols implement interning, which means uniqueness of these objects is guaranteed by the pools, so it doesn't leak into language processing logic.

The arena backing all of the aforementioned pools is a custom, general-purpose bump pointer arena implementation. It manages memory in chunks (internally named slabs), and enables access, allocation and construction through regular, high-level functions. The initial arena sizes were chosen so that typical usage doesn't have to allocate more than one chunk (though this requires some further testing and tuning, as it probably can be reduced to ease memory usage).

As a result of using arena allocation, AST nodes end up sitting adjacent to their parents in memory, since the parsers (and other AST producers) generate the AST while performing traversal recursively. This turns recursive visitation of that AST into cache-friendly memory reads thanks to the CPU pre-fetcher.

The main benefit of using a custom implementation is that it treats its size type as a template parameter. This means that type descriptors can use a 16-bit type for their handles, while nodes and symbols can use 32-bit size types. Memory management sits in one place, pools use the same template class regardless of their size type. This is further augmented by using strongly typed indexers in the pools' interface, meaning a `SymbolId` cannot be used to accidentally access a type descriptor from the type pool.

### Lightweight AST nodes

Cache-friendly AST traversal also relies on the nodes having a small memory footprint, since this is what allows larger subtrees to fit inside the L1/L2 cache.

One way this was achieved is by relying on local arena indices (IDs) instead of pointers when pointing to children nodes, type descriptors, symbols, etc. This is where having different size types for the different pools proved beneficial, since less space is taken up for representing indexes that won't occur during intended usage (like exhausting `uint16_t`'s range with unique types used in a single shader).

Another tactic is base classes explicitly allocating the size they're going to be padded to for alignment reasons. Derived classes can pack data into this space when it's small enough to fit. For example, SIR's AST node base class is the same size as its more specific expression base class, which also stores a type descriptor (through its 16-bit ID) for the given node. Additionally, the expression class still leaves 8 bits free to be used by the concrete node types, for example binary operations store their 8-bit operation kind enum value here. To avoid overcomplicating visitors with utility-related code (and accidentally introducing sneaky bugs), bitmasking logic is always abstracted behind getters/setters.
These keep error prone bitmasking math in a single place.

## Language-agnostic Type Descriptors

During transpilation, both languages mostly interact with the exact same types. These are scalars, vectors and matrices formed from scalars, structs and functions. By using a type descriptor system largely inspired by SPIR-V, types can be represented by their structure and their properties, instead of tying them to primitives per language.

The main benefit achieved with this is that a _"vector of 4 32-bit unsigned integer components"_ is the exact same type descriptor object whether it appears in Julia code, or GLSL code. The Julia frontend can query the GLSL `TargetInfo` interface directly with its own types, without any conversion. It also enables the entire transpilation pipeline to use the same type pool, and not have to maintain two different type representation systems (see [Partial Move Semantics](#partial-move-semantics)).

Since type descriptors are interned by the type pool, and the AST uses only their ID for storage, type comparison remains a single `uint16_t` equality check.

Naturally, there are some language-specific types that are useful to have during analysis, even if they never appear in the produced code (like Julia's `Nothing` type). These are also handled through the type descriptor system via builtin types, which enables languages to treat their own types however they'd like, but still have them interned and routed through the type pool.

## LLVM-style RTTI

The transpiler implements the same idea [used by LLVM](https://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html) for runtime type information.
This enables the transpiler to use a faster alternative for C++'s `dynamic_cast` and `virtual` constructs.

These were added since these are features of the language that are used extensively in hot paths across the codebase. The patterns described below aim to provide a more performant implementation for them (for this specific use case, not as general replacements), while keeping their impact on DevX minimal.

### Replacing `dynamic_cast`

Instead of relying on C++'s RTTI, which has to handle complex inheritance structures, the transpiler simply creates an exhaustive 8-bit `enum` per AST type. Each value represents a certain node kind (e.g. an `if` node), which the base node type stores. Checking if the examined node is a given node kind drops to a single integer comparison (this would typically require more complex logic via `dynamic_cast`).

A further optimization is keeping inheritance categories adjacent in the enum. For example, expression node kinds span a sequential range, and thus an _"is this node an expression?"_ check is exactly two integer comparisons.

The codebase is built with RTTI disabled, enforcing the complete abstraction of `dynamic_cast` throughout the project.

### Static Polymorphism via CRTP

The same node kind marker can also be used to speed up recursive AST traversals and provide a single `ASTVisitor` class, from which all visitors can be easily implemented (without interacting with node kind specifics at all).

Instead of relying on `virtual` for letting derived classes implement or override certain node visitors, CRTP is used, which `static_cast`s the base class' `this` pointer to the correct implementation derived type. This allows the base class to implement a single `visit` function member, which can `switch` on the given node's kind value and correctly call the concrete visitor's corresponding function member.

This approach avoids paying the `virtual` overhead per `visit` call, which is recursively called during traversal, putting it in every pass' hot path. It does so while keeping DevX unaffected when writing the language processing logic itself (implementing visitors). Using `virtual` would also require reading the vtable from memory, while this approach only reads the node kind, which is guaranteed to be a cache-hit during the node's processing.

Completely avoiding `virtual` in the codebase (except for `TargetInfo`) also means no resources in the transpiler require storing a vtable pointer. This further reduces memory size per resource (see [Lightweight AST nodes](#lightweight-ast-nodes) for why this is important).

### X-Macro System

To reduce structural duplications in the code, the `enum`s and handlers described above are generated using the X-Macro pattern, instead of appearing in the code explicitly. The AST structures are defined via `.def` files, from which the preprocessor generates the actual, lengthy `enum`s and `switch` statements needed for the aforementioned systems. Adding a new AST node kind only requires the modification of a single `.def` file, and defining its `struct` in the corresponding `ast.h`. All of the systems that use the kind values in any way will react automatically to the changes.

As a large amount of code generation can easily lead to bugs or hard-to-parse compiler errors, the codebase handles these carefully. The macros check for accidental overlaps (missing `#undef`s) and `assert`s/`static_assert`s guard branches that should be unreachable if everything is set up properly.

## Partial Move Semantics

A useful side-effect of different resources getting their own arenas (see [Arena Allocation](#arena-allocation)) in the form of pools, is that they can be reused selectively.
Each language has its own context, which it uses to manage its pools, among other things. Moving between passes for the same language needs no further memory allocation resource-wise: the types, symbols, nodes and everything else moves to the next pass as its input. T new pass is able to read them, decorate them, reorder subtrees, introduce new types via type descriptors, etc.

When moving between languages, however, some pools become meaningless: there's no use for the Julia AST (and its source location information) when the current pass is already operating in GLSL-land, its input completely disconnected from any Julia resources. The types, qualifiers, symbols, on the other hand, can all be moved between language contexts without any extra hassle.

The problem is that the lowering pass sits in the middle: it reads the Julia AST to generate its language-agnostic shader AST representation. That requires two contexts, one for the source and one for the target language.

Partial moving between contexts allows the newly constructed context to steal language-independent resources from the previous context, while leaving its language-specific resources untouched.
This means that lowering can have access to the source context and the target context at the same time. Since moving doesn't reorder the underlying arenas, all of the type IDs, symbol IDs and such will still correctly point to their corresponding resource in the new context's pools.

This allows contexts to never exist in an inconsistent state, where their own internal pools are mismatched, until they are moved, or partial moved. That is treated as the developer explicitly acknowledging responsibility for careful resource management and (in case of a partial move) taking care of discarding the old context whenever its remaining resources are no longer needed. Since the new context is constructed from the already existing pools, the AST nodes that get placed into it will point to resources that are already in the context's managed memory.
If the new context was created without the resource pools being moved immediately, it could end up in a state where its AST points to resources outside its pools. This state can now only occur after a (partial) move.

Partial moving ensures that after the context leaves the lowering pass, it cannot have cross-references between its resources that point to memory not managed by the context.
All the code juggling explicit pool lifetimes is compartmentalized into explicitly marked places in the code.
It also avoids performing unnecessary copying in memory, without moving ownership of the pools out from the context. I personally preferred keeping resource lifetimes tied to contexts, instead of managing them via globally defined pools and arenas, or introducing a pipeline-level context. This enforces the pass-level modularity of the project philosophy in the memory management model as well.
