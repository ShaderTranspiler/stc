# COMMENT: written by Claude, reviewed by szgerii

struct S
    x::Float32
end

function main()
    local x::Float32 = 1.5f0
    local v2::Vec2F = Vec2F(1)
    local v3::Vec3F = Vec3F(1)

    max(v2, v3) # ERROR: overload
    max(x, v3)  # ERROR: overload
    abs(S(x))   # ERROR: overload
end
