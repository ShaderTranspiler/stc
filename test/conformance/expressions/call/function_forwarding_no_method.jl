# COMMENT: written by Claude, reviewed by szgerii

# STC FLAG(--fwd-fns)

# COMMENT: sum exists in Julia (and isn't a GLSL builtin), but has no method for three scalars

function main()
    s = sum(1.5f0, 2.5f0, 3.5f0) # ERROR: no method matching
end
