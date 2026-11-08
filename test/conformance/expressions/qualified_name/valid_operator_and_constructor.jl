# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0

    res1 = Base.:+(x, x)     # RE OUTPUT: float res1_\d+ = \(x_\d+\) \+ \(x_\d+\);
    res2 = JuliaGLM.Vec3F(x) # RE OUTPUT: vec3 res2_\d+ = vec3\(x_\d+\);
end
