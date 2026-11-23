# COMMENT: written by Claude, reviewed by szgerii

# GLSL(missing): undeclared identifier

# COMMENT: the only use of x is inside a while body, which still makes it a capture

function f()::Float32
    x = 1.5f0

    function g()::Float32 # ERROR(missing): captured
        y = 0f0

        while true
            y = x
        end

        return y
    end

    return g()
end
