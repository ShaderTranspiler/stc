# COMMENT: Julia rounds ties to even by default, GLSL spec leaves it undefined. so it should be lowered to roundEven to match Julia semantics as closely as possible.

function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1.5f0)

    res1 = round(x)  # RE OUTPUT(missing): float res1_\d+ = roundEven\(x_\d+\);
    res2 = round.(v) # RE OUTPUT(missing): vec3 res2_\d+ = roundEven\(v_\d+\);
end
