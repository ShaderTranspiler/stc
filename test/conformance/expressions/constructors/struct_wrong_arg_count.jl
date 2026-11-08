# COMMENT: written by Claude, reviewed by szgerii

struct S
    x::Float32
    y::Float32
end

function main()
    S()                    # ERROR: invalid number of arguments
    S(1.5f0)               # ERROR: invalid number of arguments
    S(1.5f0, 2.5f0, 3.5f0) # ERROR: invalid number of arguments
end
