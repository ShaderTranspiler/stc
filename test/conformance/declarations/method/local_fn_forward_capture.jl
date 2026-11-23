# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid in Julia as long as g is only called after the assignment, but captures have to be declared before the method capturing them
# COMMENT: forward captures (generally) are another thing we could mostly resolve later, once we start doing more code rewriting

function f()::Float32
    g() = x # ERROR: forward capture
    x = 1.5f0

    return g()
end
