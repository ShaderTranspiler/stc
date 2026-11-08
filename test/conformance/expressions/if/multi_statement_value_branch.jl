# COMMENT: written by Claude, reviewed by szgerii

function main()
    local c::Bool = true

    x = if c # ERROR(missing)
        y = 1
        y + 1
    else
        3
    end
end
