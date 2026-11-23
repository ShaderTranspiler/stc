# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

# COMMENT: broadcast comparisons compare component-wise and return a bool vector, which GLSL provides builtin functions for

function main()
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    res1 = v .== w # RE OUTPUT: bvec3 res1_\d+ = equal\(v_\d+, w_\d+\);
    res2 = v .!= w # RE OUTPUT: bvec3 res2_\d+ = notEqual\(v_\d+, w_\d+\);
    res3 = v .< w  # RE OUTPUT: bvec3 res3_\d+ = lessThan\(v_\d+, w_\d+\);
    res4 = v .<= w # RE OUTPUT: bvec3 res4_\d+ = lessThanEqual\(v_\d+, w_\d+\);
    res5 = v .> w  # RE OUTPUT: bvec3 res5_\d+ = greaterThan\(v_\d+, w_\d+\);
    res6 = v .>= w # RE OUTPUT: bvec3 res6_\d+ = greaterThanEqual\(v_\d+, w_\d+\);
end
