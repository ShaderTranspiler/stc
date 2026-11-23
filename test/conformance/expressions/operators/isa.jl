# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: it's useless to support this in any manner while we have no dynamic typing

function main()
    x = 1.5f0
    b = x isa Float32 # ERROR
end
