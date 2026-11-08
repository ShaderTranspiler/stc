# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: == compares all components (or elements) and gives a single boolean, both in Julia and GLSL

function main()
    v = Vec3F(1)
    w = Vec3F(2)
    a = [1.5f0, 2.5f0]
    b = [1.5f0, 2.5f0]

    res1 = v == w # OUTPUT: bool res1
    res2 = v != w # OUTPUT: bool res2
    res3 = a == b # OUTPUT: bool res3
end
