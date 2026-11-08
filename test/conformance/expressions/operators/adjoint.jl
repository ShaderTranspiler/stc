# COMMENT: written by Claude, reviewed by szgerii

# TODO: support this in the future for scalars, vectors and matrices

function main()
    v = Vec3F(1)
    w = v' # ERROR: unsupported Expr node
end
