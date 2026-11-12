#pragma once

#include "common/bump_arena.hpp"
#include "common/id_types.hpp"
#include "types/qualifiers.hpp"

namespace stc::types {

class QualifierPool {
    using ArenaTy = BumpArena<QualId::id_type>;

    ArenaTy arena;

public:
    explicit QualifierPool(QualId::id_type initial_capacity_bytes)
        : arena{initial_capacity_bytes} {}

    QualifierPool(const QualifierPool&)            = delete;
    QualifierPool& operator=(const QualifierPool&) = delete;
    QualifierPool(QualifierPool&&)                 = default;
    QualifierPool& operator=(QualifierPool&&)      = default;

    ~QualifierPool() = default;

    ErasedArenaView get_erased_arena_view() const {
        return {
            .name    = "Type Qualifier Pool",
            .arena   = &arena,
            .size_id = ErasedArenaView::get_size_id<QualId::id_type>(),
        };
    }

    std::pair<QualId, DeclQualifiers&> emplace(std::vector<QualKind> quals,
                                               LQPayload layout_payloads);
    const DeclQualifiers& get_quals(QualId id) const;
};

} // namespace stc::types
