# COMMENT: written by Claude, reviewed by szgerii

function main()
    local iv::IVec3 = IVec3(-1)

    # COMMENT: the integer overload has to be picked, not the float one through an implicit cast
    res = abs.(iv) # RE OUTPUT(missing): ivec3 res_\d+ = abs\(iv_\d+\);
end
