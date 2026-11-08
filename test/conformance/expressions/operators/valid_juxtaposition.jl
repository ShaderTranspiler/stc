# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0

    res1 = 2x         # RE OUTPUT: float res1_\d+ = \(2(\.0*f)?\) \* \(x_\d+\);
    res2 = 2(x + 1f0) # RE OUTPUT: float res2_\d+ = \(2(\.0*f)?\) \* \(\(x_\d+\) \+ \(1\.0*f\)\);
    res3 = (x + 1f0)x # RE OUTPUT: float res3_\d+ = \(\(x_\d+\) \+ \(1\.0*f\)\) \* \(x_\d+\);
end
