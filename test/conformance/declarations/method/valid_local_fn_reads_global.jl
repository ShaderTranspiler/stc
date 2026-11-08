# COMMENT: written by Claude, reviewed by szgerii

# GLSL(missing): undeclared identifier

global x::Float32 = 1.5f0

# COMMENT: globals aren't captures, but the lifted local method has to be emitted after the global's declaration
function f()::Float32
    g() = x

    return g()
end
