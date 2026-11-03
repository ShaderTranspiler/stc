# GLSL(missing): unexpected LEFT_OP

function main()
    local i::Int32 = 1
    local u::UInt32 = 2

    # RE OUTPUT(missing): int res1_\d+ = (i_\d+) <<
    res1 = i << 2

    # RE OUTPUT(missing): uint res2_\d+ = (u_\d+) <<
    res2 = u << 2

    # RE OUTPUT(missing): int res3_\d+ = (i_\d+) >>
    res3 = i >> 2

    # RE OUTPUT(missing): uint res4_\d+ = (u_\d+) >>
    res4 = u >> 2
end
