function main()
    local v::Vec3F = Vec3F(0)

    v[2] = 1.5f0        # ERROR: immutable
    v.x  = 1.5f0        # ERROR: immutable
    v.xy = Vec2F(1.5f0) # ERROR: l-value
end
