# COMMENT: written by Claude, reviewed by szgerii

function k(x::Float32)::Float32
    return x
end

function g(x::Float32)::Float32
    return k(x)
end

function h(x::Float32)::Float32
    return k(x)
end

function f(x::Float32)::Float32
    return g(x) + h(x)
end
