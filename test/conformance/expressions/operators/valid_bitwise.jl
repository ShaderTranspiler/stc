function main()
    local x::UInt32 = 1
    local y::UInt32 = 2

    # OUTPUT: uint res1
    # OUTPUT: |
    res1 = x | y

    # OUTPUT: uint res2
    # OUTPUT: &
    res2 = x & y

    # OUTPUT: uint res3
    # OUTPUT: ^
    res3 = xor(x, y)

    # OUTPUT: uint res4
    # OUTPUT: ~
    res4 = ~x
end
