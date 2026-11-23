# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: the integer overloads have to be picked, not the float ones through implicit casts

function main()
    local i::Int32 = -2
    local j::Int32 = 3
    local u::UInt32 = UInt32(2)

    res1 = abs(i)         # RE OUTPUT(missing): int res1_\d+ = abs\(i_\d+\);
    res2 = sign(i)        # RE OUTPUT(missing): int res2_\d+ = sign\(i_\d+\);
    res3 = max(i, j)      # RE OUTPUT(missing): int res3_\d+ = max\(i_\d+, j_\d+\);
    res4 = clamp(i, 0, 1) # RE OUTPUT(missing): int res4_\d+ = clamp\(i_\d+, 0, 1\);
    res5 = min(u, u)      # RE OUTPUT(missing): uint res5_\d+ = min\(u_\d+, u_\d+\);
end
