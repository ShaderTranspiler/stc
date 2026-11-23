# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): overload

# COMMENT: Julia converts the Bool to a float (sin(true) == sin(1.0)), while GLSL's float builtins don't take bools, so the argument needs an explicit cast

function main()
    local b::Bool = true

    res1 = sin(true) # OUTPUT: float res1
    res2 = sqrt(b)   # OUTPUT: float res2
end
