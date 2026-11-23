# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): cannot convert

# COMMENT: the literal is an Int64 in Julia, but its value fits into a UInt32, so the conversion should succeed

function main()
    local u::UInt32 = 3000000000 # OUTPUT: uint u
end
