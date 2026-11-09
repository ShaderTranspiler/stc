#pragma once

#include "common/bump_arena.h"
#include "types/qualifiers.h"

namespace stc::types {

struct QualId : public StrongId<uint16_t> {
    static QualId null_id() { return QualId{0U}; }

    bool is_null() const { return value == null_id().value; }
};

class QualifierPool {
    using ArenaTy = BumpArena<QualId::id_type>;

    ArenaTy arena;

public:
    explicit QualifierPool(QualId::id_type initial_capacity_kb = 1U)
        : arena{static_cast<QualId::id_type>(initial_capacity_kb * 1024U)} {}

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
