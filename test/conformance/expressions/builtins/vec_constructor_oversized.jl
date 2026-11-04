# COMMENT: this would be valid GLSL, but JuliaGLM explicitly rejects it

function main()
    f = 1.0f0
    v2 = Vec2F(0.0f0)
    v3 = Vec3F(0.0f0)
    v4 = Vec4F(0.0f0)

    Vec2F(v3) # ERROR: oversized
    Vec2F(v4) # ERROR: oversized

    Vec3F(f, f, v2) # ERROR: oversized
    Vec4F(v3, v2)   # ERROR: oversized
    Vec4F(v3, v3)   # ERROR: oversized
    Vec4F(v3, v4)   # ERROR: oversized

    Vec4F(f, f, v3) # ERROR: oversized
    Vec4F(f, f, v4) # ERROR: oversized

    Vec4F(v2, f, v2) # ERROR: oversized
    Vec4F(v2, f, v3) # ERROR: oversized
    Vec4F(v2, f, v4) # ERROR: oversized

    Vec4F(v2, v3) # ERROR: oversized
    Vec4F(v2, v4) # ERROR: oversized
end
