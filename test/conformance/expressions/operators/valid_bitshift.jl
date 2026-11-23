# STC FLAG(-Wstrict-conformance)

# COMMENT: strict conformance because:
# COMMENT: Julia allows bit shifting by negative values, as well as values >= to the bit width, which GLSL leaves as undefined behaviour.

function main()
    local i::Int32 = 1
    local u::UInt32 = 2

    # RE OUTPUT: int res1_\d+ = \(i_\d+\) <<
    res1 = i << 2 # WARNING: STRICT CONFORMANCE

    # RE OUTPUT: uint res2_\d+ = \(u_\d+\) <<
    res2 = u << 2 # WARNING: STRICT CONFORMANCE

    # RE OUTPUT: int res3_\d+ = \(i_\d+\) >>
    res3 = i >> 2 # WARNING: STRICT CONFORMANCE

    # RE OUTPUT: uint res4_\d+ = \(u_\d+\) >>
    res4 = u >> 2 # WARNING: STRICT CONFORMANCE
end
