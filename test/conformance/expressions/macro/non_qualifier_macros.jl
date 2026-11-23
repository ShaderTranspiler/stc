# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: qualifier macros (i.e. @gl_* macros) are tested under declarations/qualifier

function main()
    a = [1, 2]
    v = Vec3F(1)

    @inbounds a[1] # ERROR: non-qualifier macro
    w = @. v + 1f0 # ERROR: non-qualifier macro
end
