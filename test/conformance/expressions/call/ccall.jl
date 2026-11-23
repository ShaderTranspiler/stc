# COMMENT: written by Claude, reviewed by szgerii

function main()
    ccall(:clock, Int32, ()) # ERROR: unsupported Expr node
end
