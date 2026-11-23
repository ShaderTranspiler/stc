# COMMENT: written by Claude, reviewed by szgerii

function main()
    local v::Vec3F = Vec3F(1)

    res1 = .-v    # RE OUTPUT: vec3 res1_\d+ = -\(v_\d+\);
    res2 = v .+ 1 # OUTPUT: vec3 res2
end
