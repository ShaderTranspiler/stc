# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: * on two vectors throws in Julia, and / on two vectors does not perform element-wise division

function main()
    local v::Vec3 = Vec3(1)
    local w::Vec3 = Vec3(2)

    res1 = v * w # ERROR(missing)
    res2 = v / w # ERROR(missing)
end
