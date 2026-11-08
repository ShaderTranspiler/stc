# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: hypot has no GLSL counterpart, so without function forwarding the call can't be resolved

function main()
    local x::Float32 = 1.5f0

    hypot(x, x) # ERROR: couldn't resolve call to 'hypot'
    # ERROR(-1): function forwarding
    # COMMENT: also ensure a hint is placed in this case into the error message
end
