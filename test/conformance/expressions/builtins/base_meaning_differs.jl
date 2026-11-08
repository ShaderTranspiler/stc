# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: Base gives these a different meaning than the GLSL builtins of the same name, so they're rejected (JuliaGLM.length, JuliaGLM.max, ... or the dotted forms give the GLSL meaning)

function main()
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    # COMMENT: the number of components in Julia
    length(v)      # ERROR(missing)

    # COMMENT: a vector of ones in Julia
    length.(v)     # ERROR(missing)

    # COMMENT: these compare the vectors as a whole and return one of them unchanged in Julia
    max(v, w)      # ERROR(missing)
    min(v, w)      # ERROR(missing)
    clamp(v, v, w) # ERROR(missing)
end
