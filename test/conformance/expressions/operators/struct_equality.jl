# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: Julia's default == on structs is ===, which compares the fields bitwise for immutable structs (NaN fields are equal, 0.0 and -0.0 aren't) and the object identity for mutable ones, unlike GLSL's field-wise ==
# COMMENT: we reject it for now all-together, since getting edge cases (e.g. NaN equality) consistent across Julia and GLSL would be tricky

struct S
    x::Float32
end

mutable struct P
    x::Float32
end

function main()
    s = S(1.5f0)
    t = S(2.5f0)
    p = P(1.5f0)
    q = P(1.5f0)

    res1 = s == t # ERROR(missing)
    res2 = s != t # ERROR(missing)
    res3 = p == q # ERROR(missing)
end
