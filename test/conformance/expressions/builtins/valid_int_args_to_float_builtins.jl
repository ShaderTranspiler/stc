# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: Julia returns Float64 here, which is coerced to Float32 by default, matching GLSL's implicit conversion to the float overload

function main()
    local i::Int32 = 2

    res1 = sin(i)  # RE OUTPUT: float res1_\d+ = sin\(i_\d+\);
    res2 = sqrt(4) # OUTPUT: float res2
end
