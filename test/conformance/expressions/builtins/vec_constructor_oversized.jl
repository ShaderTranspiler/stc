# COMMENT: this would be valid GLSL, but JuliaGLM explicitly rejects it

function main()
    v4 = Vec4F(0)
    v2 = Vec2F(v4) # ERROR: differing component count
end
