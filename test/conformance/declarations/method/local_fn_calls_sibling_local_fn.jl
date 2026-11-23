# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: local method names are locals of the enclosing method, so calling a sibling local method captures it
# COMMENT: since we're lifting local functions anyways, we could actually resolve this most of the time, but we keep it rejected for semantic consistency (for now)

function f(x::Float32)::Float32
    g(y::Float32) = y + 1.5f0
    h(y::Float32) = g(y) # ERROR: captured

    return h(x)
end
