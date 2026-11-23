# COMMENT: written by Claude, reviewed by szgerii

# STC FLAG(-Wstrict-conformance)

# COMMENT: GLSL leaves shifts by a negative count, or by the bit width or more, undefined, while Julia defines them, so every shift warns

function main()
    local i::Int32 = -8
    local u::UInt32 = UInt32(8)
    local n::Int32 = 3

    res1 = i << 33 # WARNING: STRICT CONFORMANCE
    res2 = i >> 40 # WARNING: STRICT CONFORMANCE
    res3 = u >> 32 # WARNING: STRICT CONFORMANCE
    res4 = i << -1 # WARNING: STRICT CONFORMANCE
    res5 = i << n  # WARNING: STRICT CONFORMANCE
    i <<= n        # WARNING: STRICT CONFORMANCE
end
