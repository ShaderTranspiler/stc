# COMMENT: written by Claude, reviewed by szgerii

struct S
    x::Float32
    y::Float32
end

function main()
    # COMMENT: Julia's default constructor converts its arguments to the field types

    # RE OUTPUT: S s1_\d+ =
    s1 = S(1, 2)

    # RE OUTPUT: S s2_\d+ =
    # RE OUTPUT: float\(true\)
    s2 = S(true, 2)
end
