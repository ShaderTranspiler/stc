# COMMENT: written by Claude, reviewed by szgerii

function main()
    e1 = :(1 + 2) # ERROR: unsupported Expr node

    e2 = quote # ERROR: unsupported Expr node
        1
    end
end
