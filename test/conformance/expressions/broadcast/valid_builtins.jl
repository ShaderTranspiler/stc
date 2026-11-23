# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    res1 = sin.(v)             # RE OUTPUT: vec3 res1_\d+ = sin\(v_\d+\);
    res2 = sin.(x)             # RE OUTPUT: float res2_\d+ = sin\(x_\d+\);
    res3 = clamp.(v, 0f0, 1f0) # RE OUTPUT: vec3 res3_\d+ = clamp\(v_\d+,
    res4 = max.(v, w)          # RE OUTPUT: vec3 res4_\d+ = max\(v_\d+, w_\d+\);
    res5 = fma.(v, w, v)       # RE OUTPUT: vec3 res5_\d+ = fma\(v_\d+, w_\d+, v_\d+\);
    res6 = sin.(cos.(v))       # RE OUTPUT: vec3 res6_\d+ = sin\(cos\(v_\d+\)\);
end
