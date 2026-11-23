# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: Julia's div on floats truncates the quotient, plain division in GLSL doesn't

function main()
    local x::Float32 = 7.5f0
    local y::Float32 = 2.0f0

    res1 = div(x, y) # RE OUTPUT(missing): float res1_\d+ = trunc\(
    res2 = x ÷ y     # RE OUTPUT(missing): float res2_\d+ = trunc\(
end
