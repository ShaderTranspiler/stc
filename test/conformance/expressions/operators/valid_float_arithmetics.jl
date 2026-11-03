function main()
    local x::Float32 = 1.0f0
    local y::Float32 = 1.0f0

    # COMMENT: see valid_int_arithmetics.jl for structural reasoning

    # RE OUTPUT: float res1_\d+ = \(x_\d+\) \+
    res1 = x + y
    
    # RE OUTPUT: float res2_\d+ = \(x_\d+\) -
    res2 = x - y

    # RE OUTPUT: float res3_\d+ = \(x_\d+\) \*
    res3 = x * y

    # RE OUTPUT: float res4_\d+ = \(x_\d+\) /
    res4 = x / y

    # RE OUTPUT: float res5_\d+ = -\(x_\d+\)
    res5 = -x
end
