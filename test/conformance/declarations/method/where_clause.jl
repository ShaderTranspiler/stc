# COMMENT: written by Claude, reviewed by szgerii

f(x::T) where {T} = x # ERROR: unsupported Expr node

function g(x::T) where T # ERROR: type parameters
    return x
end
