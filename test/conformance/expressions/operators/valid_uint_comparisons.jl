function main()
    local x::UInt32 = 1
    local y::UInt32 = 2

    # OUTPUT: bool res1
    # OUTPUT: ==
    res1 = x == y

    # OUTPUT: bool res2
    # OUTPUT: !=
    res2 = x != y

    # RE OUTPUT: bool res3_\d+ = \(x_\d+\) < \(y_\d+\)
    res3 = x < y

    # RE OUTPUT: bool res4_\d+ = \(x_\d+\) <= \(y_\d+\)
    res4 = x <= y

    # RE OUTPUT: bool res5_\d+ = \(x_\d+\) > \(y_\d+\)
    res5 = x > y

    # RE OUTPUT: bool res6_\d+ = \(x_\d+\) >= \(y_\d+\)
    res6 = x >= y
end

