# COMMENT: written by Claude, reviewed by szgerii

function f()::Float32
    x = 1.5f0

    function g()::Float32
        h() = x # ERROR: captured
        return h()
    end

    return g()
end
