# COMMENT: written by Claude, reviewed by szgerii

function main()
    local a::IVec3 = IVec3(1)
    local b::IVec3 = IVec3(2)

    c = a / b # RE OUTPUT: vec3 c_\d+ = \(vec3\(a_\d+\)\) / \(vec3\(b_\d+\)\);
end
