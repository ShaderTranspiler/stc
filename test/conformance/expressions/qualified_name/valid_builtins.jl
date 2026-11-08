# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1)

    res1 = Base.sin(x)        # RE OUTPUT: float res1_\d+ = sin\(x_\d+\);
    res2 = Base.Math.sin(x)   # RE OUTPUT: float res2_\d+ = sin\(x_\d+\);
    res3 = JuliaGLM.dot(v, v) # RE OUTPUT: float res3_\d+ = dot\(v_\d+, v_\d+\);
    res4 = Base.pi            # OUTPUT: float res4
end
