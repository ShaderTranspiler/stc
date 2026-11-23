# COMMENT: written by Claude, reviewed by szgerii

function main()
    local v2::Vec2F = Vec2F(1)
    local v3::Vec3F = Vec3F(1)

    res = max.(v2, v3) # ERROR: overload
end
