# COMMENT: written by Claude, reviewed by szgerii

# TODO: STC FLAG(-Wstrict-conformance)
# GLSL(missing): unexpected LEFT_OP

# COMMENT: GLSL leaves shifts by a negative count, or by the bit width or more, undefined, while Julia defines them, so every shift warns

function main()
    local i::Int32 = -8
    local u::UInt32 = UInt32(8)
    local n::Int32 = 3

    res1 = i << 33 # WARNING(missing)
    res2 = i >> 40 # WARNING(missing)
    res3 = u >> 32 # WARNING(missing)
    res4 = i << -1 # WARNING(missing)
    res5 = i << n  # WARNING(missing)
    i <<= n        # WARNING(missing)
end
