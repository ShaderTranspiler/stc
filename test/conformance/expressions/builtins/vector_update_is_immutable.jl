# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: update assignments write the component just like plain assignments, so they're rejected the same way

function main()
    local v::Vec3F = Vec3F(0)

    v.x += 1.5f0         # ERROR(missing): immutable
    v[2] += 1.5f0        # ERROR(missing): immutable
    v.xy += Vec2F(1.5f0) # ERROR(missing): immutable
end
