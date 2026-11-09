#include "api/transpiler.h"

#include "backend/glsl/code_gen.h"
#include "backend/glsl/target_info.h"
#include "external/tracy.h"
#include "frontend/jl/dumper.h"
#include "frontend/jl/lowering.h"
#include "frontend/jl/parser.h"
#include "frontend/jl/sema.h"
#include "meta.h"
#include "sir/dumper.h"
#include "types/qualifier_pool.h"

namespace stc::api {

namespace {

template <bool TrackMemoryUsage, typename CtxIdTy, typename CtxNodeBaseTy>
void init_memory_tracker(detail::MemoryTracker<TrackMemoryUsage>& memory_tracker,
                         const ASTCtx<CtxIdTy, CtxNodeBaseTy>& ctx) {
    memory_tracker.register_arena(ctx.get_node_pool_erased_arena_view());
    memory_tracker.register_arena(ctx.type_pool.get_erased_arena_view());
    memory_tracker.register_arena(ctx.sym_pool.get_erased_arena_view());
    memory_tracker.register_arena(ctx.src_info_pool.get_erased_arena_view());
    memory_tracker.register_arena(ctx.qual_pool.get_erased_arena_view());
}

// assumes the memory tracker is currently initialized to a JLCtx
template <bool TrackMemoryUsage, typename OldCtxIdTy, typename OldCtxNodeBaseTy,
          typename NewCtxIdTy, typename NewCtxNodeBaseTy>
void reinit_memory_tracker(detail::MemoryTracker<TrackMemoryUsage>& memory_tracker,
                           const ASTCtx<OldCtxIdTy, OldCtxNodeBaseTy>& old_ctx,
                           const ASTCtx<NewCtxIdTy, NewCtxNodeBaseTy>& new_ctx) {
    memory_tracker.unregister_arena(old_ctx.get_node_pool_erased_arena_view().arena);
    memory_tracker.unregister_arena(old_ctx.type_pool.get_erased_arena_view().arena);
    memory_tracker.unregister_arena(old_ctx.sym_pool.get_erased_arena_view().arena);
    memory_tracker.unregister_arena(old_ctx.src_info_pool.get_erased_arena_view().arena);
    memory_tracker.unregister_arena(old_ctx.qual_pool.get_erased_arena_view().arena);

    init_memory_tracker(memory_tracker, new_ctx);
}

} // namespace

template <bool RunBenchmark, bool TrackMemoryUsage>
MaybeString transpile_parsed(jl::NodeId jl_ast, jl::JLCtx& jl_ctx,
                             detail::BenchmarkTracker<RunBenchmark>& benchmark_tracker,
                             detail::MemoryTracker<TrackMemoryUsage>& memory_tracker) {
    using namespace stc::jl;

    auto* cmpd = jl_ctx.get_and_dyn_cast<CompoundExpr>(jl_ast);
    if (cmpd == nullptr) {
        std::cerr << "outermost AST node is not a compound expression\n";
        return std::nullopt;
    }

    // unwrap nested blocks, if any
    while (cmpd->body.size() == 1) {
        auto* inner = jl_ctx.get_and_dyn_cast<CompoundExpr>(cmpd->body[0]);

        if (inner == nullptr)
            break;

        jl_ast = jl_ctx.calculate_node_id(*inner);
        cmpd   = inner;
    }

    benchmark_tracker.sema_start();

    JLSema sema{jl_ctx, *cmpd};
    sema.infer(jl_ast);
    sema.finalize();

    benchmark_tracker.sema_end();

    if (!sema.success()) {
        std::cerr << "\nJulia sema failed\n";
        return std::nullopt;
    }

    memory_tracker.print("sema done", std::cout);

    if (jl_ctx.config.dump_sema) {
        JLDumper dumper{jl_ctx, std::cout};
        dumper.visit(jl_ast);
    }

    benchmark_tracker.lowering_start();

    JLLoweringVisitor lowering{std::move(jl_ctx)};
    auto sir_ast = lowering.lower(jl_ast);

    if (!lowering.success()) {
        std::cerr << "\nJulia -> SIR lowering failed\n";
        return std::nullopt;
    }

    auto glsl_ctx = glsl::GLSLCtx(std::move(lowering.sir_ctx));

    benchmark_tracker.lowering_end();

    reinit_memory_tracker(memory_tracker, jl_ctx, glsl_ctx);
    memory_tracker.print("lowering done", std::cout);

    if (glsl_ctx.config.dump_lowered) {
        sir::SIRDumper dumper{glsl_ctx, std::cout};
        dumper.visit(sir_ast);
    }

    benchmark_tracker.code_gen_start();

    glsl::GLSLCodeGenVisitor code_gen_vis{glsl_ctx};
    code_gen_vis.visit(sir_ast);

    if (!code_gen_vis.success()) {
        std::cerr << "\nGLSL code generation failed\n";
        return std::nullopt;
    }

    benchmark_tracker.code_gen_end();
    benchmark_tracker.end();
    memory_tracker.print("all done", std::cout);

    benchmark_tracker.print(std::cout);

    return code_gen_vis.move_result();
}

template STC_API MaybeString transpile_parsed<true, true>(jl::NodeId, jl::JLCtx&,
                                                          detail::BenchmarkTracker<true>&,
                                                          detail::MemoryTracker<true>&);
template STC_API MaybeString transpile_parsed<false, true>(jl::NodeId, jl::JLCtx&,
                                                           detail::BenchmarkTracker<false>&,
                                                           detail::MemoryTracker<true>&);
template STC_API MaybeString transpile_parsed<true, false>(jl::NodeId, jl::JLCtx&,
                                                           detail::BenchmarkTracker<true>&,
                                                           detail::MemoryTracker<false>&);
template STC_API MaybeString transpile_parsed<false, false>(jl::NodeId, jl::JLCtx&,
                                                            detail::BenchmarkTracker<false>&,
                                                            detail::MemoryTracker<false>&);

template <bool RunBenchmark, bool TrackMemoryUsage>
MaybeString transpile(std::string_view code, std::optional<std::string_view> file_path,
                      stc::TranspilerConfig config, std::string_view juliaglm_path) {
    using namespace stc::jl;

    detail::BenchmarkTracker<RunBenchmark> benchmark_tracker{};
    detail::MemoryTracker<TrackMemoryUsage> memory_tracker{};

    benchmark_tracker.start();
    benchmark_tracker.init_start();

    JLCtx jl_ctx{juliaglm_path};
    jl_ctx.config = std::move(config);

    glsl::GLSLTargetInfo target_info{jl_ctx.type_pool};
    jl_ctx.target_info = &target_info;

    benchmark_tracker.init_end();

    init_memory_tracker(memory_tracker, jl_ctx);
    memory_tracker.print("initial", std::cout);

    benchmark_tracker.parser_start();

    JLParser parser{jl_ctx};

    if (file_path.has_value())
        parser.fallback_file = *file_path;

    NodeId jl_ast = parser.parse_code(code);

    if (!parser.success()) {
        std::cerr << "\nJulia parser failed\n";
        return std::nullopt;
    }

    benchmark_tracker.parser_end();
    memory_tracker.print("parser done", std::cout);

    if (jl_ctx.config.dump_parsed) {
        JLDumper dumper{jl_ctx, std::cout};
        dumper.visit(jl_ast);
    }

    return transpile_parsed<RunBenchmark, TrackMemoryUsage>(jl_ast, jl_ctx, benchmark_tracker,
                                                            memory_tracker);
}

template STC_API MaybeString transpile<true, true>(std::string_view,
                                                   std::optional<std::string_view>,
                                                   stc::TranspilerConfig, std::string_view);
template STC_API MaybeString transpile<true, false>(std::string_view,
                                                    std::optional<std::string_view>,
                                                    stc::TranspilerConfig, std::string_view);
template STC_API MaybeString transpile<false, true>(std::string_view,
                                                    std::optional<std::string_view>,
                                                    stc::TranspilerConfig, std::string_view);
template STC_API MaybeString transpile<false, false>(std::string_view,
                                                     std::optional<std::string_view>,
                                                     stc::TranspilerConfig, std::string_view);

template <bool RunBenchmark, bool TrackMemoryUsage>
MaybeString transpile(jl_value_t* expr_v, stc::TranspilerConfig config,
                      std::string_view juliaglm_path) {
    using namespace stc::jl;

    detail::BenchmarkTracker<RunBenchmark> benchmark_tracker{};
    detail::MemoryTracker<TrackMemoryUsage> memory_tracker{};

    benchmark_tracker.start();
    benchmark_tracker.init_start();

    JLCtx jl_ctx{juliaglm_path};
    jl_ctx.config = std::move(config);

    glsl::GLSLTargetInfo target_info{jl_ctx.type_pool};
    jl_ctx.target_info = &target_info;

    benchmark_tracker.init_end();

    init_memory_tracker(memory_tracker, jl_ctx);
    memory_tracker.print("initial", std::cout);

    benchmark_tracker.parser_start();

    JLParser parser{jl_ctx};

    NodeId jl_ast = parser.parse(expr_v);

    if (!parser.success()) {
        std::cerr << "\nJulia parser failed\n";
        return std::nullopt;
    }

    benchmark_tracker.parser_end();
    memory_tracker.print("parser done", std::cout);
    if (jl_ctx.config.dump_parsed) {
        JLDumper dumper{jl_ctx, std::cout};
        dumper.visit(jl_ast);
    }

    return transpile_parsed<RunBenchmark, TrackMemoryUsage>(jl_ast, jl_ctx, benchmark_tracker,
                                                            memory_tracker);
}

template STC_API MaybeString transpile<true, true>(jl_value_t*, stc::TranspilerConfig,
                                                   std::string_view);
template STC_API MaybeString transpile<true, false>(jl_value_t*, stc::TranspilerConfig,
                                                    std::string_view);
template STC_API MaybeString transpile<false, true>(jl_value_t*, stc::TranspilerConfig,
                                                    std::string_view);
template STC_API MaybeString transpile<false, false>(jl_value_t*, stc::TranspilerConfig,
                                                     std::string_view);

namespace {

template <typename TranspileFn>
requires std::invocable<TranspileFn&, stc::TranspilerConfig&> &&
         std::same_as<std::invoke_result_t<TranspileFn&, stc::TranspilerConfig&>, std::string*>
void* invoke_transpilation(void* cfg_handle, TranspileFn&& transpile_fn) noexcept {
    try {
        stc::TranspilerConfig config = cfg_handle != nullptr
                                           ? *(static_cast<stc::TranspilerConfig*>(cfg_handle))
                                           : stc::TranspilerConfig{};

        return static_cast<void*>(transpile_fn(config));
    } catch (const std::exception& ex) {
        std::cerr << "the following std::exception was thrown during transpilation:\n";
        std::cerr << ex.what() << '\n';
    } catch (...) {
        std::cerr << "an unexpected error was thrown during transpilation\n";
    }

    return nullptr;
}

// ugly, but for 2 bools gets the job done.
// CLEANUP (especially if we want to do more profiling args like this)
template <typename... Args>
auto dispatch_transpilation(bool run_benchmarks, bool track_memory_usage, Args&&... args) {
    if (run_benchmarks)
        return track_memory_usage ? stc::api::transpile<true, true>(std::forward<Args>(args)...)
                                  : stc::api::transpile<true, false>(std::forward<Args>(args)...);
    else
        return track_memory_usage ? stc::api::transpile<false, true>(std::forward<Args>(args)...)
                                  : stc::api::transpile<false, false>(std::forward<Args>(args)...);
}

} // namespace

extern "C" {
    STC_API uint8_t stc_abi_version() noexcept {
        return stc::meta::version_major;
    }

    STC_API void* stc_transpile(jl_value_t* expr_v, bool run_benchmark, bool track_memory_usage,
                                void* cfg_handle) noexcept {
        if (!jl_is_expr(expr_v)) {
            std::cerr << "non-Expr julia object cannot be transpiled\n";
            return nullptr;
        }

        return invoke_transpilation(cfg_handle, [&](TranspilerConfig& config) {
            auto result = dispatch_transpilation(run_benchmark, track_memory_usage, expr_v, config);

            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            return result.has_value() ? new std::string(std::move(*result)) : new std::string{};
        });
    }

    STC_API void* stc_transpile_code(const char* code, bool run_benchmark, bool track_memory_usage,
                                     void* cfg_handle) noexcept {
        if (code == nullptr) {
            std::cerr << "nullptr passed to stc_transpile_code as the input code\n";
            return nullptr;
        }

        return invoke_transpilation(cfg_handle, [&](TranspilerConfig& config) {
            auto result = dispatch_transpilation(run_benchmark, track_memory_usage, code,
                                                 std::nullopt, config);

            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            return result.has_value() ? new std::string(std::move(*result)) : new std::string{};
        });
    }

    // gets the underlying C string data of a result handle
    STC_API const char* stc_get_result(void* result_handle) noexcept {
        if (result_handle == nullptr) {
            std::cerr << "nullptr passed to stc_get_result\n";
            return nullptr;
        }

        return static_cast<std::string*>(result_handle)->c_str();
    }

    // frees the underlying string data belonging to a result handle
    STC_API void stc_free_result(void* result_handle) noexcept {
        if (result_handle == nullptr) {
            std::cerr << "nullptr passed to stc_free_result\n";
            return;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete static_cast<std::string*>(result_handle);
    }
}

} // namespace stc::api
