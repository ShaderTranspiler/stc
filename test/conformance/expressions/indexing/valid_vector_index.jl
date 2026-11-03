function main()
    local i::Int32 = 2
    local u::UInt32 = 2
    v = Vec3F(1.5f0)

    x1 = v[1] # RE OUTPUT: float x1_\d+ = v_\d+\[\(1\) - \(1\)\];
    x2 = v[3] # RE OUTPUT: float x2_\d+ = v_\d+\[\(3\) - \(1\)\];
    x3 = v[i] # RE OUTPUT: float x3_\d+ = v_\d+\[\(i_\d+\) - \(1\)\];
    x4 = v[u] # RE OUTPUT: float x4_\d+ = v_\d+\[\(u_\d+\) - \(1u\)\];
end
