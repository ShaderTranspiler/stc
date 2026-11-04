function main()
    f  = 1.0f0
    v2 = Vec2F(f)
    v3 = Vec3F(f)
    v4 = Vec4F(f)

    Vec2F(f, f)
    Vec3F(f, f, f)
    Vec4F(f, f, f, f)

    Vec2F(v2)
    Vec3F(v3)
    Vec4F(v4)

    Vec3F(v2, f)
    Vec3F(f, v2)
    Vec3F(f, f, v2)

    Vec4F(v2, f, f)
    Vec4F(f, v2, f)
    Vec4F(f, f, v2)

    Vec4F(f, v3)
    Vec4F(v3, f)
    Vec4F(v3, v2)
    Vec4F(v3, v3)
    Vec4F(v3, v4)

    Vec4F(f, f, v2)
    Vec4F(f, f, v3)
    Vec4F(f, f, v4)

    Vec4F(v2, f, v2)
    Vec4F(v2, f, v3)
    Vec4F(v2, f, v4)

    Vec4F(v2, v2)
    Vec4F(v2, v3)
    Vec4F(v2, v4)
end
