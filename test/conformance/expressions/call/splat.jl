# COMMENT: written by Claude, reviewed by szgerii

# SKIP: stc crashes on calls with an argument that failed to parse

f(x::Int32, y::Int32) = x

function main()
    a = [1, 2]
    f(a...) # ERROR: unsupported Expr node
end
