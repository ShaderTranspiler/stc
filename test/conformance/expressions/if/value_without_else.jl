# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: the missing else branch evaluates to nothing

function main()
    local c::Bool = true

    x = if c # ERROR: non-materializable
        1
    end
end
