function main()
    local f::Float32 = 1.5f0
    local v::Vec3F = Vec3F(2.0f0)

    v + f # ERROR(missing)
    f + v # ERROR(missing)
    v - f # ERROR(missing)
    f - v # ERROR(missing)

    # COMMENT: Julia returns a 1x3 row vector here, not an element-wise result
    f / v # ERROR(missing)
end
