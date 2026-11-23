# COMMENT: written by Claude, reviewed by szgerii

mutable struct S
    x::Float32
end

# RE OUTPUT: f_\d+\(inout S s_\d+\)
function f(@gl_inout s::S)
    s.x = 1.5f0
end

# RE OUTPUT: g_\d+\(inout S a_\d+, inout S b_\d+\)
function g(@gl_inout(a::S), @gl_inout(b::S))
    a.x = b.x
end

# RE OUTPUT: h_\d+\(float y_\d+, inout S s_\d+\)
function h(y::Float32, @gl_inout s::S)
    s.x = y
end

function main()
    a = S(0f0)
    b = S(1.5f0)

    f(a)
    g(a, b)
    h(2.5f0, a)
end
