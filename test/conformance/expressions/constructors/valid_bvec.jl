# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): BVec3

function main()
    local b::BVec3 = BVec3(true, false, true) # OUTPUT: bvec3 b
    c = b.x                                   # OUTPUT: bool c
    d = b[2]                                  # OUTPUT: bool d
end
