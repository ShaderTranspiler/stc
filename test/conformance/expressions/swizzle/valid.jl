function main()
    local f::Float32 = 0.0
    v2 = Vec2(0)
    v3 = Vec3(0)
    v4 = Vec4(0)

    f = v2.x
    v2 = v2.xy
    v3 = v3.xyz
    v4 = v4.xyzw
    
    v4 = v3.zyxx
    v2 = v4.zw
    
    dv2 = DVec2(0)
    dv3 = DVec3(0)

    dv3 = dv2.yxy
end
