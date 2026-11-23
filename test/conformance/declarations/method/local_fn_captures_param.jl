# COMMENT: written by Claude, reviewed by szgerii

function f(x::Float32)::Float32
    g() = x # ERROR: captured

    return g()
end
