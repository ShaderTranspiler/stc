# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): overload

# COMMENT: Julia's max and min take any number of arguments, they're folded into nested binary calls

function main()
    local x::Float32 = 1.5f0
    local y::Float32 = 2.5f0
    local z::Float32 = 3.5f0
    local i::Int32 = 1

    res1 = max(x, y, z)    # RE OUTPUT: float res1_\d+ = max\(max\(x_\d+, y_\d+\), z_\d+\);
    res2 = min(x, y, z, x) # OUTPUT: float res2
    res3 = max(i, x, y)    # OUTPUT: float res3
end
