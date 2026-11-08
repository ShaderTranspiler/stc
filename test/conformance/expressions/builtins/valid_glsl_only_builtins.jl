# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

# COMMENT: Base doesn't define these (or defines step for something unrelated), JuliaGLM's GLSL-style versions are called qualified

function main()
    local x::Float32 = 1.5f0

    res1 = JuliaGLM.fract(x)             # OUTPUT: float res1
    res2 = JuliaGLM.mix(x, 2.5f0, 0.5f0) # OUTPUT: float res2
    res3 = JuliaGLM.step(1f0, x)         # OUTPUT: float res3
end
