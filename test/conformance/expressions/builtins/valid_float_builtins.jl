# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0
    local y::Float32 = 2.5f0

    res1 = sin(x)             # OUTPUT: float res1
    res2 = sign(x)            # OUTPUT: float res2
    res3 = atan(y, x)         # RE OUTPUT: float res3_\d+ = atan\(y_\d+, x_\d+\);
    res4 = fma(x, y, x)       # OUTPUT: float res4
    res5 = clamp(x, 0f0, 1f0) # OUTPUT: float res5
    res6 = isnan(x)           # OUTPUT: bool res6
end
