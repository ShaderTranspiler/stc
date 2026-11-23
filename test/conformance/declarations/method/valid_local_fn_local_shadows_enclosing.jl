# COMMENT: written by Claude, reviewed by szgerii

function f()::Float32
    x = 1.5f0

    function g()::Float32
        local x = 2.5f0
        return x
    end

    return g() + x
end
