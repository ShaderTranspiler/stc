# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: assigning to a local of the enclosing method captures it in Julia, it doesn't declare a new local

function f()::Float32
    x = 1.5f0

    function g() # ERROR: captured
        x = 2.5f0
    end

    g()
    return x
end
