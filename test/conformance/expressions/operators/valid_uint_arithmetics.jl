function main()
    local x::UInt32 = 0
    local y::UInt32 = 0

    # COMMENT: see valid_int_arithmetics.jl for structural reasoning

    # RE OUTPUT: uint res1_\d+ = \(x_\d+\) \+
    res1 = x + y

    # RE OUTPUT: uint res2_\d+ = \(x_\d+\) -
    res2 = x - y

    # RE OUTPUT: uint res3_\d+ = \(x_\d+\) \*
    res3 = x * y

    # RE OUTPUT: uint res4_\d+ = -\(x_\d+\)
    res4 = -x

    # RE OUTPUT: uint res5_\d+ = \(x_\d+\) %
    res5 = x % y

    # RE OUTPUT: uint res6_\d+ = \(x_\d+\) %
    res6 = rem(x, y)

    # RE OUTPUT: uint res7_\d+ = \(x_\d+\) /
    res7 = div(x, y)
end
