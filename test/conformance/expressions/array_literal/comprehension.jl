# COMMENT: written by Claude, reviewed by szgerii

function main()
    a = [i for i in 1:3]        # ERROR: unsupported Expr node
    b = Float32[i for i in 1:3] # ERROR: unsupported Expr node
end
