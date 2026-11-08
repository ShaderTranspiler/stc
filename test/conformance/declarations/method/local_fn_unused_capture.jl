# COMMENT: written by Claude, reviewed by szgerii

function f()::Float32
    x = 1.5f0
    g() = x # ERROR: captured

    return x
end
