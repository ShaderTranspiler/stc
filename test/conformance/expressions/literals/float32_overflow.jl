# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: these don't fit into a Float32, which they're coerced to by default

function main()
    x = 1e40  # ERROR(missing)
    y = -1e40 # ERROR(missing)
end
