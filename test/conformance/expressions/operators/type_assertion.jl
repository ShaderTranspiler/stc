# COMMENT: written by Claude, reviewed by szgerii

function main()
    x = 1.5f0
    x::Float32 # ERROR: unsupported Expr node
end
