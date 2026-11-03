function main()
    local x::Int32 = 1
    local y::Int32 = 2

    # COMMENT: new vars introduced to also enforce result type based on inference

    # RE OUTPUT: int res1_\d+ = \(x_\d+\) \+
    res1 = x + y

    # RE OUTPUT: int res2_\d+ = \(x_\d+\) -
    res2 = x - y

    # RE OUTPUT: int res3_\d+ = \(x_\d+\) \*
    res3 = x * y

    # RE OUTPUT: int res4_\d+ = -\(x_\d+\)
    res4 = -x

    # RE OUTPUT: int res5_\d+ = \(x_\d+\) %
    res5 = x % y

    # RE OUTPUT: int res6_\d+ = \(x_\d+\) %
    res6 = rem(x, y)

    # RE OUTPUT: int res7_\d+ = \(x_\d+\) /
    res7 = div(x, y)
end
