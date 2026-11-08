# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

# COMMENT: JuliaGLM's GLSL-style functions take whole vectors, they're called qualified (unqualified names refer to Base)

function main()
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    res1 = JuliaGLM.max(v, w)    # RE OUTPUT: vec3 res1_\d+ = max\(v_\d+, w_\d+\);
    res2 = JuliaGLM.dot(v, w)    # OUTPUT: float res2
    res3 = JuliaGLM.cross(v, w)  # OUTPUT: vec3 res3
    res4 = JuliaGLM.normalize(v) # OUTPUT: vec3 res4
    res5 = JuliaGLM.length(v)    # RE OUTPUT: float res5_\d+ = length\(v_\d+\);
end
