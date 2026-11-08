# COMMENT: written by Claude, reviewed by szgerii

# SKIP: stc crashes on calls with an argument that failed to parse

function main()
    s = sum(i for i in 1:3) # ERROR: unsupported Expr node
end
