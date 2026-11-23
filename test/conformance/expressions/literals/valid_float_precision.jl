# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: small values and values with many significant digits have to keep their precision

function main()
    a = 1f-10        # RE OUTPUT: float a_\d+ = 1(\.0*)?e-0?10f;
    b = 1.2345678f-5 # RE OUTPUT: float b_\d+ = 1\.2345678e-0?5f;
    c = 0.1f0        # RE OUTPUT: float c_\d+ = 0\.1f;
end
