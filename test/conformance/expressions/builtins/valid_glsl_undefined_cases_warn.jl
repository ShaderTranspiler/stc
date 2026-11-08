# COMMENT: written by Claude, reviewed by szgerii

# TODO: STC FLAG(-Wstrict-conformance)

# COMMENT: Julia defines these for every input, GLSL leaves some cases undefined or decides them differently (atan with both arguments zero, clamp with lo > hi, max and min with a NaN or with signed zeros), so we warn under -Wstrict-conformance

function main()
    local x::Float32 = 0.5f0
    local y::Float32 = 1.5f0

    res1 = atan(y, x)         # WARNING(missing)
    res2 = clamp(x, 0f0, 1f0) # WARNING(missing)
    res3 = max(x, y)          # WARNING(missing)
    res4 = min(x, y)          # WARNING(missing)
end
