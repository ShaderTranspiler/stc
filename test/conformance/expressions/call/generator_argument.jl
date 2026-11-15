# COMMENT: written by Claude, reviewed by szgerii

function main()
    s = sum(i for i in 1:3) # ERROR: unsupported Expr node
end
