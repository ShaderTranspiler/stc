# COMMENT: written by Claude, reviewed by szgerii

# SKIP: stc crashes on calls with an argument that failed to parse

function main()
    ccall(:clock, Int32, ()) # ERROR: unsupported Expr node
end
