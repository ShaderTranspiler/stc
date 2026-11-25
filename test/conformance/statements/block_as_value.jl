# COMMENT: written by Claude, reviewed by szgerii

function main()
    x = begin # ERROR(+1): single expression is expected
        y = 1
        y + 1
    end

    z = (w = 1; w + 1) # ERROR: single expression is expected
end
