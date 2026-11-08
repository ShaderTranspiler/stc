# COMMENT: written by Claude, reviewed by szgerii

f(x::Int32) = x

function main()
    f(1) do y # ERROR: unsupported Expr node
        y
    end
end
