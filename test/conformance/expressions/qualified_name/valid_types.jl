# COMMENT: written by Claude, reviewed by szgerii

# STC(missing, internal)

f(x::Base.Int32) = x

function main()
    local x::Base.Float32 = 1.5f0      # OUTPUT: float x
    local v::JuliaGLM.Vec3F = Vec3F(1) # OUTPUT: vec3 v

    n = f(1) # OUTPUT: int n
end
