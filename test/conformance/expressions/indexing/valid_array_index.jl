function main()
    a = [1.5f0, 2.5f0, 3.5f0]
    local i::Int32 = 2
    local u::UInt32 = 2

    x1 = a[1] # RE OUTPUT: float x1_\d+ = a_\d+\[\(1\) - \(1\)\];
    x2 = a[i] # RE OUTPUT: float x2_\d+ = a_\d+\[\(i_\d+\) - \(1\)\];
    x3 = a[u] # RE OUTPUT: float x3_\d+ = a_\d+\[\(u_\d+\) - \(1u\)\];
end
