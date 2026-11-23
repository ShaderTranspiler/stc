# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: these broadcast into the target in place, which throws in Julia for both vectors (immutable) and scalars (no in-place broadcast)

function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    v .+= w   # ERROR(missing)
    x .+= 1f0 # ERROR(missing)
end
