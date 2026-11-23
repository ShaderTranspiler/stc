# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: the result type would depend on the branch taken at runtime in Julia, so both branches need the same type

function main()
    local c::Bool = true

    x = c ? 1 : 1.5f0 # ERROR

    y = if c # ERROR
        Int32(1)
    else
        true
    end
end
