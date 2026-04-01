#include "frontend/jl/scope.h"
#include "frontend/jl/dumper.h"

namespace {

using namespace stc::jl;

std::string scope_kind_str(ScopeKind kind) {
    switch (kind) {
        case ScopeKind::Global:
            return "global";

        case ScopeKind::Hard:
            return "hard";

        case ScopeKind::Soft:
            return "soft";
    }

    throw std::logic_error{"Unaccounted ScopeKind in scope_kind_str"};
}

std::string binding_type_str(BindingType bt) {
    switch (bt) {
        case BindingType::Global:
            return "global";

        case BindingType::Captured:
            return "captured";

        case BindingType::Local:
            return "local";
    }

    throw std::logic_error{"Unaccounted BindingType in binding_type_str"};
}

} // namespace

namespace stc::jl {

void JLScope::dump(const JLCtx& ctx, std::ostream& out) const {
    out << "==============================\n";
    out << "scope snapshot debug dump\n\n";

    out << std::format("kind: {}\n\n", scope_kind_str(kind));

    out << "binding table:\n";

    for (auto [sym_id, bt] : binding_table)
        out << std::format(" \\/ {} -> {}\n", ctx.get_sym(sym_id), binding_type_str(bt));

    out << "\n\nsymbol table:\n";

    JLDumper dumper{ctx, std::cout};

    for (auto [sym_id, decl_id] : symbol_table) {
        out << std::format(" \\/ {} -> \n", ctx.get_sym(sym_id));
        dumper.visit(decl_id);
    }

    out << "\n\ndeferred methods:\n";

    for (NodeId method_decl : methods)
        dumper.visit(method_decl);

    out << "\n==============================\n";
}

} // namespace stc::jl
