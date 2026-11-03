function main()
    local v::Vec3 = Vec3(1)
    local w::Vec3 = Vec3(2)

    res1 = v + w # OUTPUT: vec3 res1
    res2 = v - w # OUTPUT: vec3 res2
    res3 = v * w # OUTPUT: vec3 res3
    res4 = v / w # OUTPUT: vec3 res4

    res5 = v .+ w # OUTPUT: vec3 res5
    res6 = v .- w # OUTPUT: vec3 res6
    res7 = v .* w # OUTPUT: vec3 res7
    res8 = v ./ w # OUTPUT: vec3 res8
end
