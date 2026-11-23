# COMMENT: written by Claude, reviewed by szgerii

f(x::Int32, y::Int32) = x

function main()
    a = [1, 2]
    f(a...) # ERROR: unsupported Expr node
end
