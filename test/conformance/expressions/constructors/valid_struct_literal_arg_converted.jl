# COMMENT: written by Claude, reviewed by szgerii

struct S
    u::UInt32
end

# COMMENT: the literal fits into a UInt32, so it converts like in Julia

function main()
    s = S(1) # OUTPUT: S s
end
