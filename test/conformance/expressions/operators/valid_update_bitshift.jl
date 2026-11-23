# STC FLAG(-Wstrict-conformance)

# COMMENT: strict conformance because:
# COMMENT: Julia allows bit shifting by negative values, as well as values >= to the bit width, which GLSL leaves as undefined behaviour.

function main()
    local i::Int32 = 1
    local u::UInt32 = 2

    # OUTPUT: <<=
    # OUTPUT: >>=
    i <<= 2 # WARNING: STRICT CONFORMANCE
    i >>= 2 # WARNING: STRICT CONFORMANCE
    u <<= 2 # WARNING: STRICT CONFORMANCE
    u >>= 2 # WARNING: STRICT CONFORMANCE
end
