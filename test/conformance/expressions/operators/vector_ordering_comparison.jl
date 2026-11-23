# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: Julia compares vectors from left to right, based on the first differing component. GLSL has no operator that matches that (the broadcast form is separate, and lowers to component-wise comparison)

function main()
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    res1 = v < w  # ERROR
    res2 = v >= w # ERROR
end
