# COMMENT: written by Claude, reviewed by szgerii

# GLSL(missing): unexpected IF

g(x::Float32) = x

function h(c::Bool)::Float32
    return c ? 1.5f0 : 2.5f0
end

function main()
    local c::Bool = true
    local d::Bool = false

    res1 = g(c ? 1.5f0 : 2.5f0)
    res2 = h(c)
    res3 = c ? (d ? 1 : 2) : 3         # OUTPUT: int res3
    res4 = (c ? 1.0f0 : 2.0f0) + 1.0f0 # OUTPUT: float res4
end
