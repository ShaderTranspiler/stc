# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

# COMMENT: Base doesn't define these (or defines step for something unrelated), GLUtils's GLSL-style versions are called qualified

function main()
    local x::Float32 = 1.5f0

    res1 = GLUtils.fract(x)             # OUTPUT: float res1
    res2 = GLUtils.mix(x, 2.5f0, 0.5f0) # OUTPUT: float res2
    res3 = GLUtils.step(1f0, x)         # OUTPUT: float res3
end
