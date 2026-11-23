# COMMENT: written by Claude, reviewed by szgerii

function main()
    x = begin # ERROR(missing)
        y = 1
        y + 1
    end

    z = (w = 1; w + 1) # ERROR(missing)
end
