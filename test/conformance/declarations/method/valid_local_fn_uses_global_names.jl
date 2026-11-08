# COMMENT: written by Claude, reviewed by szgerii

struct S
    x::Float32
end

h() = 1.5f0

# COMMENT: global methods and structs aren't captures
function f()::Float32
    g() = h() + S(2.5f0).x

    return g()
end
