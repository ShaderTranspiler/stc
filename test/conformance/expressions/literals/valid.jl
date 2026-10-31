function main()
    # COMMENT: bool
    true
    false
    
    # COMMENT: Int64 (coercable to Int32)
    0
    30
    -12
    2147483647
    -2147483648
    1_000_000
    10_00_00_0

    # COMMENT: UInt32
    0x00001
    0x00000001
    0xffffffff

    # COMMENT: Float32
    0f0
    1.5f0
    1.f0
    .5f0
    -2.125f0
    12.5f-3
    12.5f+3

    # COMMENT: Float64 (coercable to Float32)
    0.0
    1.5
    1.
    .5
    -2.125
    12.5e3
    12.5e-3
end
