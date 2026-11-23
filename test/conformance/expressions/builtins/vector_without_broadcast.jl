# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: these builtins only apply element-wise to vectors through broadcasting in Julia

function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1)

    sin(v)             # ERROR(missing)
    max(v, x)          # ERROR(missing)
    clamp(v, 0f0, 1f0) # ERROR(missing)
    mod(v, 2f0)        # ERROR(missing)
end
