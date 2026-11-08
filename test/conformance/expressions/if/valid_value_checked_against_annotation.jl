# COMMENT: written by Claude, reviewed by szgerii

# GLSL(missing): unexpected IF

# COMMENT: with an annotated target, each branch is converted to the annotated type on its own

function main()
    local c::Bool = true
    local y::Float32 = c ? 1 : 2.5f0
end
