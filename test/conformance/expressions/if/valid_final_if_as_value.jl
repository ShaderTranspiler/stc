# GLSL(missing): unexpected IF

# COMMENT: the final if (or ternary) is the method's value, so it's an if used as a value

function g(c::Bool)::Int32
    a = 1
    b = 0

    if c
        a
    else
        b
    end
end

function h(c::Bool)::Int32
    a = 1
    b = 0

    c ? a : b
end
