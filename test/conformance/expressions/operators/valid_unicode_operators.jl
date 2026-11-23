# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: this list is obviously not exhaustive, but these aren't handled as special cases, they go through the exact same operator handling logic as everything else, so a handful of cases should be fine for this

function main()
    local x::Float32 = 1.5f0
    local i::Int32 = 7

    res1 = x ≤ 2f0 # RE OUTPUT: bool res1_\d+ = \(x_\d+\) <= \(2\.0*f\);
    res2 = x ≥ 2f0 # RE OUTPUT: bool res2_\d+ = \(x_\d+\) >= \(2\.0*f\);
    res3 = x ≠ 2f0 # RE OUTPUT: bool res3_\d+ = \(x_\d+\) != \(2\.0*f\);
    res4 = i ⊻ 2   # RE OUTPUT: int res4_\d+ = \(i_\d+\) \^ \(2\);
end
