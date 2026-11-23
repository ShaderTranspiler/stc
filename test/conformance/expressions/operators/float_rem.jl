# COMMENT: Julia's rem and % for floats needs wrapper code generated when lowering to GLSL

function main()
    local x::Float32 = 1.0f0
    local y::Float32 = 2.0f0

    # COMMENT: for now, we should just reject it
    # ERROR(+1): rem
    x % y     # ERROR(missing): floating-point
    # ERROR(+1): rem
    rem(x, y) # ERROR(missing): floating-point
end
