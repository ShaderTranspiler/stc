# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid Julia, but the return would end up inside an operand of the generated ternary in GLSL, requires some code lifting/rewriting

function f(c::Bool)::Int32
    x = c ? (return Int32(1)) : 2 # ERROR
    return x
end
