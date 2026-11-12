#include "stc/stc.h"
#include "stc/stc.hpp"

#include <cstdio>
#include <utility>

#include "backend/glsl/code_gen.hpp"
#include "backend/glsl/target_info.hpp"
#include "external/tracy.hpp"
#include "frontend/jl/dumper.hpp"
#include "frontend/jl/lowering.hpp"
#include "frontend/jl/parser.hpp"
#include "frontend/jl/rt/utils.hpp"
#include "frontend/jl/sema.hpp"
#include "sir/dumper.hpp"

namespace stc::api {

namespace {

// ================
//     C++ API
//    (helpers)
// ================

template <typename CtxIdTy, typename CtxNodeBaseTy>
void init_memory_tracker(MemoryTracker& memory_tracker, const ASTCtx<CtxIdTy, CtxNodeBaseTy>& ctx) {
    memory_tracker.register_arena(ctx.get_node_pool_erased_arena_view());
    memory_tracker.register_arena(ctx.type_pool.get_erased_arena_view());
    memory_tracker.register_arena(ctx.sym_pool.get_erased_arena_view());
    memory_tracker.register_arena(ctx.src_info_pool.get_erased_arena_view());
    memory_tracker.register_arena(ctx.qual_pool.get_erased_arena_view());
}

// since the node pool doesn't get moved at the same time the shared pools do, its unregistration
// isn't handled here
template <typename CtxIdTy, typename CtxNodeBaseTy>
void uninit_memory_tracker(MemoryTracker& memory_tracker, const ASTCtx<CtxIdTy, CtxNodeBaseTy>& ctx,
                           bool include_node_pool) {
    // since MemoryTracker might come from the user and not have the context's pools registered, we
    // don't wanna throw if the arena wasn't tracked to begin with.
    // if they were registered by the user, we have to remove them, since we just moved them.

    if (include_node_pool)
        memory_tracker.unregister_arena(ctx.get_node_pool_erased_arena_view().arena, false);
    memory_tracker.unregister_arena(ctx.type_pool.get_erased_arena_view().arena, false);
    memory_tracker.unregister_arena(ctx.sym_pool.get_erased_arena_view().arena, false);
    memory_tracker.unregister_arena(ctx.src_info_pool.get_erased_arena_view().arena, false);
    memory_tracker.unregister_arena(ctx.qual_pool.get_erased_arena_view().arena, false);
}

// this is a bit iffy, but it allows us to avoid a heap alloc for the trackers, allow callers to
// pass in their own one for recording (without copy) and doesn't pollute the public API
// this is safe because transpilations are currently not allowed to be interleaved within a single
// thread (and even multithreaded usage is not guaranteed to work).
// also, initializing a disabled (false) tracker is pretty cheap
void ensure_trackers(TimingTracker*& timing_tracker, MemoryTracker*& memory_tracker,
                     const TranspilerConfig& config) {
    static thread_local TimingTracker timing_tracker_storage(false);
    static thread_local MemoryTracker memory_tracker_storage(false);

    if (timing_tracker == nullptr) {
        timing_tracker_storage = TimingTracker(config.track_timing);
        timing_tracker         = &timing_tracker_storage;
    }

    if (memory_tracker == nullptr) {
        memory_tracker_storage = MemoryTracker(config.track_memory);
        memory_tracker         = &memory_tracker_storage;
    }
}

} // namespace

// ================
//     C++ API
// (implementation)
// ================

namespace {

TranspilationResult transpile_impl(jl::NodeId jl_ast, jl::JLCtx& jl_ctx,
                                   TimingTracker* timing_tracker, MemoryTracker* memory_tracker) {
    using namespace stc::jl;

    ensure_trackers(timing_tracker, memory_tracker, jl_ctx.config);
    assert(timing_tracker != nullptr);
    assert(memory_tracker != nullptr);

    auto timer = timing_tracker->create_timer();

    auto* cmpd = jl_ctx.get_and_dyn_cast<CompoundExpr>(jl_ast);
    if (cmpd == nullptr) {
        error("outermost AST node is not a compound expression\n");
        return TranspilationResult::failed_result;
    }

    // unwrap nested blocks, if any
    while (cmpd->body.size() == 1) {
        auto* inner = jl_ctx.get_and_dyn_cast<CompoundExpr>(cmpd->body[0]);
        if (inner == nullptr)
            break;

        jl_ast = jl_ctx.calculate_node_id(*inner);
        cmpd   = inner;
    }

    timer.start("Julia sema");

    JLSema sema{jl_ctx, *cmpd};
    sema.infer(jl_ast);
    sema.finalize();

    if (!sema.success()) {
        std::cerr << "\nJulia sema failed\n";
        return sema.had_internal_error() ? TranspilationResult::internal_error_result
                                         : TranspilationResult::failed_result;
    }

    timer.end();
    memory_tracker->print("after sema", std::cout);

    if (jl_ctx.config.dump_sema) {
        JLDumper dumper{jl_ctx, std::cout};
        dumper.visit(jl_ast);
    }

    // unregisters shared pools which are about to get moved
    // Julia node pool stays, since we only partial moved, and jl_ctx lives at least until the end
    // of this function. unregistering it is the caller's responsibility (both internally and
    // externally), since they're the ones managing the jl_ctx's lifetime
    uninit_memory_tracker(*memory_tracker, jl_ctx, false);
    timer.start("AST lowering");

    auto node_pool_init = jl_ctx.config.initial_pool_sizes.sir_node_pool;
    JLLoweringVisitor lowering{std::move(jl_ctx), node_pool_init};
    auto sir_ast = lowering.lower(jl_ast);

    if (!lowering.success()) {
        std::cerr << "\nJulia -> SIR lowering failed\n";
        return lowering.had_internal_error() ? TranspilationResult::internal_error_result
                                             : TranspilationResult::failed_result;
    }

    auto glsl_ctx = glsl::GLSLCtx(std::move(lowering.sir_ctx));

    timer.end(); // includes initing the GLSL context

    // registers shared pools and SIR node pool
    init_memory_tracker(*memory_tracker, glsl_ctx);
    // might result in a harmless double unregister, see transpile(jl_value_t*, ...)
    ScopeGuard uninit_guard{[&]() { uninit_memory_tracker(*memory_tracker, glsl_ctx, true); }};

    memory_tracker->print("after lowering", std::cout);

    if (glsl_ctx.config.dump_lowered) {
        sir::SIRDumper dumper{glsl_ctx, std::cout};
        dumper.visit(sir_ast);
    }

    timer.start("Code generation");

    glsl::GLSLCodeGenVisitor code_gen_vis{glsl_ctx};
    code_gen_vis.visit(sir_ast);

    if (!code_gen_vis.success()) {
        std::cerr << "\nGLSL code generation failed\n";
        return code_gen_vis.had_internal_error() ? TranspilationResult::internal_error_result
                                                 : TranspilationResult::failed_result;
    }

    timer.end();
    memory_tracker->print("all done", std::cout);

    timing_tracker->print(std::cout);

    return {
        .code   = code_gen_vis.move_result(),
        .status = Status::Success,
    };
}

TranspilationResult transpile_impl(jl_value_t* expr_v, const stc::TranspilerConfig& config,
                                   TimingTracker* timing_tracker, MemoryTracker* memory_tracker) {
    using namespace stc::jl;

    ensure_trackers(timing_tracker, memory_tracker, config);
    assert(timing_tracker != nullptr);
    assert(memory_tracker != nullptr);

    auto timer = timing_tracker->create_timer();

    timer.start("Resource init");

    const auto& init_sizes = config.initial_pool_sizes;
    JLCtx jl_ctx{config.julia_glm_path, init_sizes.julia_node_pool, init_sizes.source_info_pool,
                 init_sizes.type_pool,  init_sizes.symbol_pool,     init_sizes.qualifier_pool};

    // CLEANUP: check if we can avoid this copy freely by making a context's config non-owning
    jl_ctx.config = config;

    glsl::GLSLTargetInfo glsl_target_info{jl_ctx.type_pool};
    jl_ctx.target_info = &glsl_target_info;

    timer.end();

    init_memory_tracker(*memory_tracker, jl_ctx);
    // this might result in shared pools getting unregistered twice (once by us and once by the
    // post-parser overload), but that's fine since uninit_memory_tracker passes false to
    // unregister_tracker, making a redundant removal a no-op
    // this way we're sure to not leak any dead resources to the caller through the memory tracker
    ScopeGuard tracker_uninit_guard{
        [&]() { uninit_memory_tracker(*memory_tracker, jl_ctx, true); }};

    memory_tracker->print("after resource init", std::cout);

    timer.start("Expr parser");

    JLParser parser{jl_ctx};

    // even though the parser burns the fallback file path (if needed) into the source location
    // info, and we never read it again during transpilation, we copy here instead of moving, since
    // the config itself might be reused by the end user between transpilations!
    if (config.fallback_file_path.has_value())
        parser.fallback_file = *config.fallback_file_path;

    NodeId jl_ast = parser.parse(expr_v);

    if (!parser.success()) {
        std::cerr << "\nJulia parser failed\n";
        return parser.had_internal_error() ? TranspilationResult::internal_error_result
                                           : TranspilationResult::failed_result;
    }

    timer.end();
    memory_tracker->print("after parser", std::cout);

    if (jl_ctx.config.dump_parsed) {
        JLDumper dumper{jl_ctx, std::cout};
        dumper.visit(jl_ast);
    }

    return transpile_impl(jl_ast, jl_ctx, timing_tracker, memory_tracker);
}

TranspilationResult transpile_impl(std::string_view code, const stc::TranspilerConfig& config,
                                   TimingTracker* timing_tracker, MemoryTracker* memory_tracker) {
    using namespace stc::jl;

    ensure_trackers(timing_tracker, memory_tracker, config);
    assert(timing_tracker != nullptr);
    assert(memory_tracker != nullptr);

    auto timer = timing_tracker->create_timer();

    // OPTIMIZE: we currently only have utils for function lookup via the whole runtime env
    // interface and its subcomponents (like the module cache), so we're being a bit wasteful
    // here and initializing a temporary module cache to run the lookup logic
    rt::JuliaModuleCache temp_module_cache{config.julia_glm_path};

    jl_value_t* expr_v = nullptr;
    JL_GC_PUSH1(&expr_v);
    const ScopeGuard jl_gc_pop_guard{[]() { JL_GC_POP(); }};

    timer.start("Julia's parser", false);
    expr_v = rt::parse_code(code, temp_module_cache);
    timer.end();

    if (expr_v == nullptr) {
        std::cerr << "\nFailed to run Meta.parse on the source Julia code";
        return TranspilationResult::failed_result;
    }

    return transpile_impl(expr_v, config, timing_tracker, memory_tracker);
}

} // namespace

// ==============
//   Shared API
//   (helpers)
// ==============

namespace {

template <typename Fn>
requires std::same_as<std::invoke_result_t<Fn>, void>
void wrap_as_noexcept(Fn&& fn) noexcept {
    try {
        std::forward<Fn>(fn)();
    } catch (const std::exception& ex) {
        // doesn't use a helper like try_report_error since fmt::format exceptions would have to
        // be caught separately anyways
        try {
            stc::internal_error(fmt::format("exception thrown during transpilation:\n"
                                            "{}",
                                            ex.what()));
        } catch (...) {
            // ex.what() is noexcept, so this is fine
            fprintf(stderr,
                    "[Internal Error] exception thrown during transpilation:\n"
                    "%s\n",
                    ex.what());
        }
    } catch (...) {
        // same logic as with a concrete exception
        try {
            stc::internal_error("non-exception throw during transpilation!");
        } catch (...) {
            fprintf(stderr, "[Internal Error] non-exception throw during transpilation!\n");
        }
    }
}

// performs some copying on the return value, only use with cheap to copy types!
template <typename Fn>
requires (!std::same_as<std::invoke_result_t<Fn>, void>) &&
         std::is_copy_constructible_v<std::invoke_result_t<Fn>> &&
         std::is_copy_assignable_v<std::invoke_result_t<Fn>>
auto wrap_as_noexcept(Fn&& fn, std::invoke_result_t<Fn> value_on_throw) noexcept {
    using RetTy = std::invoke_result_t<Fn>;

    auto run_wrapped = [&fn, &value_on_throw]() {
        RetTy return_value(std::move(value_on_throw));
        wrap_as_noexcept([&fn, &return_value]() { return_value = std::forward<Fn>(fn)(); });
        return return_value;
    };

    if constexpr (std::is_nothrow_move_constructible_v<RetTy> &&
                  std::is_nothrow_copy_assignable_v<RetTy>) {
        return run_wrapped();
    }

    try {
        return run_wrapped();
    } catch (...) {
        // this can only occur if move constructing, or copy assigning RetTy throws
        // besides getting this information out, we don't do much further reporting
        fprintf(
            stderr,
            "[Internal Error] Error thrown while handling return value in a wrap_as_noexcept call");
        return value_on_throw;
    }
}

} // namespace

// ===============
//     C++ API
//   (endpoints)
// ===============

STC_API TranspilationResult transpile(std::string_view code, const stc::TranspilerConfig& config,
                                      TimingTracker* timing_tracker,
                                      MemoryTracker* memory_tracker) noexcept {
    return wrap_as_noexcept(
        [=, &config]() { return transpile_impl(code, config, timing_tracker, memory_tracker); },
        TranspilationResult::internal_error_result);
}

STC_API TranspilationResult transpile(jl_value_t* expr_v, const stc::TranspilerConfig& config,
                                      TimingTracker* timing_tracker,
                                      MemoryTracker* memory_tracker) noexcept {
    return wrap_as_noexcept(
        [=, &config]() { return transpile_impl(expr_v, config, timing_tracker, memory_tracker); },
        TranspilationResult::internal_error_result);
}

STC_API TranspilationResult transpile(jl::NodeId jl_ast, jl::JLCtx& jl_ctx,
                                      TimingTracker* timing_tracker,
                                      MemoryTracker* memory_tracker) noexcept {
    return wrap_as_noexcept(
        [=, &jl_ctx]() { return transpile_impl(jl_ast, jl_ctx, timing_tracker, memory_tracker); },
        TranspilationResult::internal_error_result);
}

// ===========
//    C API
//  (helpers)
// ===========

namespace {

// assumes msg is null-terminated (since it's used with literals only)
void try_report_error(std::string_view msg) noexcept {
    // we try to use our custom error format first, but that might still throw
    // fprintf is C-lib, so it can't throw (if it does error, we don't print any output, that's fine
    // at that point, we're just trying to quit then without an exception to satisfy C rules)
    // because we're already reporting an error, the exception here doesn't matter, we just (...) it
    try {
        stc::error(msg);
    } catch (...) {
        assert(*(msg.data() + msg.length()) == '\0');
        fprintf(stderr, "[Error] %s\n", msg.data());
    }
}

const stc::TranspilerConfig& get_config_from_handle(STC_ConfigHandle cfg_handle) {
    static const stc::TranspilerConfig default_config{};
    return cfg_handle != STC_NULL_CONFIG ? *static_cast<const stc::TranspilerConfig*>(cfg_handle)
                                         : default_config;
}

STC_Status process_transpilation_result(TranspilationResult& result,
                                        STC_ResultHandle& result_handle) {
    // if we don't get to the end of the function (e.g. bad alloc), default to null result
    result_handle = STC_NULL_RESULT;

    if (result.status == Status::Success) {
        auto* code_ptr = new std::string(std::move(result.code));
        result_handle  = code_ptr;
    }

    return static_cast<STC_Status>(result.status);
}

} // namespace

// ===========
//    C API
// (endpoints)
// ===========

extern "C" {
    STC_API STC_Status stc_transpile(jl_value_t* expr_v, STC_ConfigHandle cfg_handle,
                                     STC_ResultHandle* result_handle) noexcept {
        return wrap_as_noexcept(
            [expr_v, cfg_handle, &result_handle]() -> STC_Status {
                if (result_handle == nullptr) {
                    try_report_error("NULL passed as result_handle in a call to stc_transpile");
                    return STC_FAILED;
                }

                // treat any unexpected exception as an internal error
                *result_handle = STC_NULL_RESULT;

                if (expr_v == nullptr) {
                    try_report_error("NULL passed as expr_v in a call to stc_transpile");
                    return STC_FAILED;
                }

                if (!jl_is_expr(expr_v)) {
                    try_report_error("non-Expr Julia object cannot be transpiled");
                    return STC_FAILED;
                }

                auto result = transpile(expr_v, get_config_from_handle(cfg_handle));
                return process_transpilation_result(result, *result_handle);
            },
            STC_INTERNAL_ERROR);
    }

    STC_API STC_Status stc_transpile_code(const char* code, STC_ConfigHandle cfg_handle,
                                          STC_ResultHandle* result_handle) noexcept {
        return wrap_as_noexcept(
            [code, cfg_handle, &result_handle]() -> STC_Status {
                if (result_handle == nullptr) {
                    try_report_error(
                        "NULL passed as result_handle in a call to stc_transpile_code");
                    return STC_FAILED;
                }

                // treat any unexpected exception from here as an internal error
                *result_handle = STC_NULL_RESULT;

                if (code == nullptr) {
                    try_report_error("NULL passed as code in a call to stc_transpile_code");
                    return STC_FAILED;
                }

                auto result = transpile(code, get_config_from_handle(cfg_handle));
                return process_transpilation_result(result, *result_handle);
            },
            STC_INTERNAL_ERROR);
    }

    // gets the underlying C string data of a result handle
    STC_API const char* stc_get_result(STC_ResultHandle result_handle) noexcept {
        if (result_handle == STC_NULL_RESULT)
            return nullptr;

        return static_cast<std::string*>(result_handle)->c_str();
    }

    // frees the underlying string data belonging to a result handle
    STC_API void stc_free_result(STC_ResultHandle result_handle) noexcept {
        if (result_handle == STC_NULL_RESULT)
            return;

        delete static_cast<std::string*>(result_handle);
    }
}

} // namespace stc::api
