# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: currently we separate const and @gl_const (which shifts constness responsibility to the user because we have no proper constness checking mechanism yet)

const x::Float32 = 1.5f0 # ERROR: unsupported Expr node
