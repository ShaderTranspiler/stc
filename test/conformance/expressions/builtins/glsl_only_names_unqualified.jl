# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: these aren't defined in Julia without qualification (step is, but only for ranges), JuliaGLM's GLSL-style versions have to be called as JuliaGLM.dot, JuliaGLM.mix, ...

function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    dot(v, w)                # ERROR(missing)
    cross(v, w)              # ERROR(missing)
    normalize(v)             # ERROR(missing)
    mix(x, 2.5f0, 0.5f0)     # ERROR(missing)
    fract(x)                 # ERROR(missing)
    smoothstep(0f0, 1f0, x)  # ERROR(missing)
    step(1f0, x)             # ERROR(missing)
end
