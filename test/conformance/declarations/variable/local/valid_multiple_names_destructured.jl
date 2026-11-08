# COMMENT: written by Claude, reviewed by szgerii

# STC(missing, internal)

function main()
    # RE OUTPUT: float a_\d+
    # RE OUTPUT: int b_\d+
    local a, b = 1.5f0, 2

    # RE OUTPUT: float c_\d+
    # RE OUTPUT: int d_\d+
    local c::Float32, d::Int32 = 1, 2
end
