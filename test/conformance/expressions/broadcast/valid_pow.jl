# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

function main()
    local v::Vec3F = Vec3F(1)

    res1 = v .^ 2   # OUTPUT: vec3 res1
    res2 = v .^ 2f0 # OUTPUT: vec3 res2
end
