# COMMENT: written by Claude, reviewed by szgerii

g = 0f0

# COMMENT: g has no declared type, so assigning a value of another type would change its type

function main()
    global g
    g = Int32(1) # ERROR(missing)
end
