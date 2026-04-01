#include "julia.h"
JULIA_DEFINE_FAST_TLS

#include <backend/glsl/code_gen.h>
#include <frontend/jl/dumper.h>
#include <frontend/jl/lowering.h>
#include <frontend/jl/parser.h>
#include <frontend/jl/sema.h>
#include <sir/dumper.h>
#include <sir/sema.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[]) {
    using namespace stc;
    using namespace stc::jl;
    using clock = std::chrono::steady_clock;

    // ! TODO: remove
    std::ifstream file(argc > 1 ? argv[1] : "C:\\Users\\szucs\\szakdoga\\stc\\test.jl");
    std::stringstream code_stream;
    code_stream << file.rdbuf();
    std::string code{code_stream.str()};

    jl_init();

    auto start = clock::now();

    JLParser parser{};
    NodeId jl_ast = parser.parse_code(code);
    JLCtx ctx{parser.steal_ctx()};

    auto* cmpd = ctx.get_and_dyn_cast<CompoundExpr>(jl_ast);
    if (cmpd == nullptr)
        throw std::logic_error{"outermost node is not a compound expression"};

    JLSema sema{ctx, *cmpd};
    sema.infer(jl_ast);

    if (!sema.success()) {
        std::cerr << "Julia sema failed" << std::endl;
        return 1;
    }

    // JLDumper jl_dumper{ctx, std::cout};
    // jl_dumper.visit(jl_ast);

    JLLoweringVisitor lowering{std::move(ctx)};
    auto sir_ast = lowering.visit(jl_ast);

    if (!lowering.successful()) {
        std::cerr << "Julia -> SIR lowering failed" << std::endl;
        return 1;
    }

    auto glsl_ctx = glsl::GLSLCtx(std::move(lowering.sir_ctx));

    // sir::SIRDumper sir_dumper{glsl_ctx, std::cout};
    // sir_dumper.visit(sir_ast);

    // sir::SIRSemaVisitor sema_vis{glsl_ctx};
    // sema_vis.visit(sir_ast);
    // if (!sema_vis.success()) {
    //     std::cerr << "SIR semantic verification failed" << std::endl;
    //     return 1;
    // }

    glsl::GLSLCodeGenVisitor code_gen_vis{glsl_ctx};
    code_gen_vis.visit(sir_ast);

    if (!code_gen_vis.successful()) {
        std::cerr << "GLSL code gen failed" << std::endl;
        return 1;
    }

    auto end = clock::now();

    std::ofstream out_file{"out.comp"};
    out_file << code_gen_vis.result();
    out_file.flush();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << std::format("\nTranspilation finished in {}\n", duration) << std::endl;

    jl_atexit_hook(0);
    return 0;
}
