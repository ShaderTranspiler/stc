# COMMENT: written by Claude, reviewed by szgerii

# TODO: STC FLAG(-Wstrict-conformance)

# COMMENT: Julia throws an InexactError when the value doesn't fit the target type (a fraction, out of range, NaN, a negative value into an unsigned type), GLSL truncates or reinterprets instead, so these conversions should warn under -Wstrict-conformance

function main()
    local x::Float32 = 1.5f0
    local i::Int32   = -1
    local u::UInt32  = UInt32(7)

    res1 = Int32(x)  # WARNING(missing)
    res2 = UInt32(x) # WARNING(missing)
    res3 = UInt32(i) # WARNING(missing)
    res4 = Int32(u)  # WARNING(missing)
end
