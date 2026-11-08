# COMMENT: written by Claude, reviewed by szgerii

mutable struct S
    x::Float32
end

# COMMENT: Julia parses the second parameter into the first macro call, the parenthesized form @gl_inout(b::S) has to be used

# ERROR(+1, missing): @gl_inout(
function f(@gl_inout a::S, @gl_inout b::S) # INTERNAL ERROR: unexpected parameter layout
    a.x = b.x
end
