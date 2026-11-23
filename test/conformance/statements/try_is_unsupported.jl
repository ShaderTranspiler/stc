# COMMENT: written by Claude, reviewed by szgerii

function main()
    try # ERROR: unsupported Expr node
        x = 1
    catch
        x = 2
    end
end
