#include <external/julia.h>
JULIA_DEFINE_FAST_TLS

#include <api/stc.hpp>
#include <common/config.h>
#include <common/report.h>
#include <common/term_utils.h>
#include <common/utils.h>
#include <frontend/jl/utils.h>

#include "cli_utils.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>

#define STC_CHECK_EXCEPTIONS                                                                       \
    if (jl::check_exceptions()) {                                                                  \
        std::cerr << "\nAn error occured while initializing Julia\n";                              \
        return EXIT_FAILURE;                                                                       \
    }

#define STC_EVAL_AND_CHECK(str)                                                                    \
    jl_eval_string(str);                                                                           \
    STC_CHECK_EXCEPTIONS

namespace {

std::optional<std::string> open_julia_file_as_cmpd(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Couldn't open Julia file at '" << path << "'\n";
        return std::nullopt;
    }

    std::stringstream code_stream;
    code_stream << "begin ";
    code_stream << file.rdbuf();
    // new line before end so that trailing comments are not a problem
    code_stream << "\nend";
    return std::move(code_stream).str();
}

void print_conflicting_flags_warning(std::string_view flag1, std::string_view flag2,
                                     std::string_view suffix = "") {
    std::cerr << stc::colored("Warning: ", ansi_codes::yellow)
              << "conflicting flags have been provided as arguments: "
              << stc::colored(flag1, ansi_codes::cyan) << " and "
              << stc::colored(flag2, ansi_codes::cyan) << ".\n";

    if (!suffix.empty())
        std::cerr << stc::colored(suffix, ansi_codes::yellow) << '\n';
}

} // namespace

namespace stc::cli {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
int run(int argc, char* argv[]) {
    using namespace stc;

    setup_process();

    if (argc < 2) {
        std::cerr << stc::colored("Not enough arguments\n", ansi_codes::red);
        std::cerr << "Usage: stc <input file> [OPTIONS]\n";
        std::cerr << "Run with " << stc::colored("--help", ansi_codes::yellow) << " or "
                  << stc::colored("-h", ansi_codes::yellow) << " for a list of available options\n";
        return EXIT_FAILURE;
    }

    std::string src_file_path = argv[1];

    // only help and version info can be invoked on their own
    // for every other case, arguments must start with the input path

    if (src_file_path == "-h" || src_file_path == "--help") {
        print_help();
        return EXIT_SUCCESS;
    }
    if (src_file_path == "-v" || src_file_path == "--version") {
        print_version_info();
        return EXIT_SUCCESS;
    }

    TranspilerConfig config{};
    config.track_timing       = true;
    config.fallback_file_path = tail_from_cwd(src_file_path).string();

    std::string out_path{};
    std::string pre_eval_path{};
    uint32_t ite_count = 1U;
    bool no_out        = false;
    bool quiet         = false;
    bool add_no_commit = true;

    bool explicitly_asked_for_timing = false;
    for (int i = 2; i < argc; i++) {
        std::string arg{argv[i]};

        if (arg == "-h" || arg == "--help") {
            print_help();
            return EXIT_SUCCESS;
        }

        if (arg == "-v" || arg == "--version") {
            print_version_info();
            return EXIT_SUCCESS;
        }

        if (arg == "--no-timing") {
            config.track_timing         = false;
            explicitly_asked_for_timing = false;
        } else if (arg == "--timing") {
            config.track_timing         = true;
            explicitly_asked_for_timing = true;
        } else if (arg == "--track-memory")
            config.track_memory = true;
        else if (arg == "--quiet")
            quiet = true;
        else if (arg == "--dump-parsed")
            config.dump_parsed = true;
        else if (arg == "--dump-sema")
            config.dump_sema = true;
        else if (arg == "--dump-lowered")
            config.dump_lowered = true;
        else if (arg == "--dump-scopes")
            config.dump_scopes = true;
        else if (arg == "--spaces")
            config.use_tabs = false;
        else if (arg == "--tabs")
            config.use_tabs = true;
        else if (arg == "--no-out")
            no_out = true;
        else if (arg == "--fwd-fns")
            config.forward_fns = true;
        else if (arg == "--conv-fail-reason")
            config.print_convert_fail_reason = true;
        else if (arg == "--track-bindings")
            config.track_bindings = true;
        else if (arg == "--no-coerce-i32")
            config.coerce_to_i32 = false;
        else if (arg == "--no-coerce-f32")
            config.coerce_to_f32 = false;
        else if (arg == "--no-coercion") {
            config.coerce_to_i32 = false;
            config.coerce_to_f32 = false;
        } else if (arg == "--no-uniform-capture")
            config.capture_uniforms = false;
        else if (arg == "-Wjl-query")
            config.warn_on_jl_sema_query = true;
        else if (arg == "--skip-no-commit")
            add_no_commit = false;
        else if (arg == "--error-dump") {
            if (i + 1 >= argc) {
                std::cerr << "--error-dump must be followed by a verbosity level (options: none, "
                             "partial, verbose)\n";
                return EXIT_FAILURE;
            }

            std::string_view verbosity{argv[i + 1]};

            if (verbosity == "none")
                config.error_dump_verbosity = DumpVerbosity::None;
            else if (verbosity == "partial")
                config.error_dump_verbosity = DumpVerbosity::Partial;
            else if (verbosity == "verbose")
                config.error_dump_verbosity = DumpVerbosity::Verbose;
            else {
                std::cerr << fmt::format("'{}' is not a valid error dumping verbosity level "
                                         "(options: none, partial, verbose)\n",
                                         verbosity);
                return EXIT_FAILURE;
            }

            i++;
        } else if (arg == "--it") {
            if (i + 1 >= argc) {
                std::cerr << "--it must be followed by the number of iterations\n";
                return EXIT_FAILURE;
            }

            using IteTy = std::remove_cvref_t<decltype(ite_count)>;

            std::string next_arg{argv[i + 1]};
            auto maybe_ite_count = try_parse_num<IteTy>(next_arg);

            if (!maybe_ite_count.has_value()) {
                std::cerr << fmt::format(
                    "--it followed by an invalid argument (expected range: {}-{})\n",
                    std::numeric_limits<IteTy>::min(), std::numeric_limits<IteTy>::max());

                return EXIT_FAILURE;
            }

            ite_count = *maybe_ite_count;
            i++;
        } else if (arg == "--cg-indent") {
            if (i + 1 >= argc) {
                std::cerr
                    << "--cg-indent must be followed by the width of indentation (in spaces)\n";
                return EXIT_FAILURE;
            }

            using CgIndentTy = std::remove_cvref_t<decltype(config.code_gen_indent)>;

            auto maybe_cg_indent = try_parse_num<CgIndentTy>(std::string{argv[i + 1]});

            if (!maybe_cg_indent.has_value()) {
                std::cerr << fmt::format(
                    "--cg-indent followed by an invalid argument (expected range: {}-{})\n",
                    std::numeric_limits<CgIndentTy>::min(), std::numeric_limits<CgIndentTy>::max());

                return EXIT_FAILURE;
            }

            config.code_gen_indent = *maybe_cg_indent;
            i++;
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "-o must be followed by the output file's path\n";
                return EXIT_FAILURE;
            }

            out_path = argv[i + 1];
            i++;
        } else if (arg == "--gl-version") {
            if (i + 1 >= argc) {
                std::cerr << "--gl-version must be followed by the desired OpenGL version\n";
                return EXIT_FAILURE;
            }

            config.target_version = std::string{argv[i + 1]};
            i++;
        } else if (arg == "--pre-eval") {
            if (i + 1 >= argc) {
                std::cerr << "--pre-eval must be followed by the path of a Julia file\n";
                return EXIT_FAILURE;
            }

            pre_eval_path = std::string{argv[i + 1]};
            i++;
        } else {
            std::cerr << fmt::format("Unknown argument: {}\n", arg);
            return EXIT_FAILURE;
        }
    }

    if (no_out && !out_path.empty())
        print_conflicting_flags_warning("--no-out", fmt::format("-o {}", out_path),
                                        "No output will be written after transpilation.");

    if (!add_no_commit && out_path.empty()) {
        std::cerr << stc::colored("Warning: ", ansi_codes::yellow);
        std::cerr << stc::colored("--skip-no-commit", ansi_codes::cyan) << " without "
                  << stc::colored("-o", ansi_codes::cyan) << ".\n";
        std::cerr << "A no"
                     "commit tag is only added to output written to disk, so it has no effect if "
                  << stc::colored("-o", ansi_codes::cyan) << " is not provided along with it.\n";
    }

    if (quiet && !explicitly_asked_for_timing)
        config.track_timing = false;

    if (!quiet)
        print_header(std::cout, "Pre-transpilation (CLI-only work)");

    if (!quiet)
        std::cout << "Reading Julia code from disk...\n";

    auto maybe_code = open_julia_file_as_cmpd(src_file_path);
    if (!maybe_code.has_value())
        return EXIT_FAILURE;
    std::string code = std::move(*maybe_code);

    if (!quiet)
        std::cout << "Initializing a new Julia instance for transpilation...\n";

    jl_init();
    STC_CHECK_EXCEPTIONS

    const ScopeGuard jl_guard{[]() { jl_atexit_hook(0); }};

    if (!quiet)
        std::cout << "Importing JuliaGLM...\n";

    STC_EVAL_AND_CHECK("using JuliaGLM");

    if (!pre_eval_path.empty()) {
        if (!std::filesystem::exists(pre_eval_path)) {
            std::cerr << fmt::format(
                "No file exists at the path provided for the pre-eval script ('{}')\n",
                pre_eval_path);
            return EXIT_FAILURE;
        }

        if (!quiet)
            std::cout << "Running pre-eval Julia script...\n";

        jl_value_t* include_fn_ptr = jl_get_function(jl_base_module, "include");
        if (include_fn_ptr == nullptr) {
            std::cerr << "Couldn't locate include function inside the Base module\n";
            return EXIT_FAILURE;
        }

        jl_value_t* path_jl_str = jl_cstr_to_string(pre_eval_path.c_str());

        jl_call2(include_fn_ptr, reinterpret_cast<jl_value_t*>(jl_main_module), path_jl_str);

        // doesn't use the macro to provide a custom error msg
        if (jl::check_exceptions()) {
            std::cerr << "An error occurred while running the pre-eval script\n";
            return EXIT_FAILURE;
        }
    }

    bool print_per_transpile_header =
        ite_count > 1 && (config.track_timing || config.track_memory || !quiet);

    if (!print_per_transpile_header && !quiet)
        print_header(std::cout, "Transpilation", true);

    for (size_t i = 0; i < ite_count; i++) {
        if (print_per_transpile_header)
            print_header(std::cout, fmt::format("Transpilation #{}", i + 1), !quiet);

        if (!quiet)
            std::cout << "Starting transpilation...\n";

        auto result = api::transpile(code, config);

        if (result.status != stc::api::Status::Success) {
            std::cerr << colored("\nTranspilation failed.\n", ansi_codes::red);
            return EXIT_FAILURE;
        }

        if (!quiet)
            std::cout << colored("\nTranspilation finished successfully.\n", ansi_codes::green);

        if (!no_out && i + 1 == ite_count) {
            if (out_path.empty()) {
                if (!quiet) {
                    print_header(std::cout,
                                 ite_count == 1 ? "Output" : "Output (last transpilation)");
                }

                std::cout << result.code << '\n';
                break;
            }

            if (!quiet)
                print_header(std::cout, "Post-transpilation (CLI-only work)");

            // gotta do C-style file writing, because libjulia messes with std::locale in a way that
            // breaks std::ofstream in some release builds

            if (!quiet)
                std::cout << fmt::format("Writing output to {}...\n", out_path);

            // Make sure that when using a git pre-commit hook that rejects committing the
            // no-commit substring (real form without hyphen, written this way to avoid tripping the
            // actual hook), generated output can't be included by accident, since the CLI is mostly
            // used for debugging.
            // This is a CLI-only feature (doesn't affect the transpiler itself), and can be turned
            // off when saving the output long-term is desired, via --skip-no-commit. the sample
            // pre-commit hook (scripts/pre-commit) checks for this tag by default
            if (add_no_commit) {
                // we already have a trailing newline in result, so this is placed in its own line
                // it's written this way so that it doesn't trip the actual hook
                result.code += "// no"
                               "commit\n";
            }

            stc::FileRAII out_file{out_path, "w"};
            if (out_file.get() == nullptr) {
                std::cerr << colored(fmt::format("\nFailed to open '{}' for writing! (errno: {})\n",
                                                 out_path, errno),
                                     ansi_codes::red);
                return EXIT_FAILURE;
            }

            size_t written = fwrite(result.code.data(), 1, result.code.size(), out_file.get());
            if (written != result.code.size()) {
                std::cerr << colored("\nFailed to write output to file!\n", ansi_codes::red);
                return EXIT_FAILURE;
            }

            if (!quiet)
                std::cout << colored("\nOutput written successfully.\n", ansi_codes::green);
        }
    }

#ifdef STC_PROFILING
    std::cout << "Profiling enabled, waiting for key press so that Tracy can flush data "
                 "properly...\n";
    std::cin.get();
#endif

    return EXIT_SUCCESS;
}

} // namespace stc::cli

int main(int argc, char* argv[]) {
    try {
        return stc::cli::run(argc, argv);
    } catch (const std::exception& ex) {
        fprintf(stderr, "An unexpected error occurred while running stc:\n%s\n", ex.what());
    } catch (...) {
        fprintf(stderr, "An unexpected error occurred while running stc!\n");
    }

    return EXIT_FAILURE;
}

#undef STC_EVAL_AND_CHECK
#undef STC_CHECK_EXCEPTIONS
