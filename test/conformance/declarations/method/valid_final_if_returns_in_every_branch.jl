# COMMENT: written by Claude, reviewed by szgerii

# GLSL(missing): unexpected IF

function f(c::Bool)::Int32
    a = 1
    b = 0

    if c
        return a
    else
        return b
    end
end
