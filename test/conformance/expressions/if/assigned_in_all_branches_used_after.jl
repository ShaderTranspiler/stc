# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: x is assigned on every path, but it's still declared inside the branches, so it's rejected the same way as an assignment in a single branch
# TODO: in the future, we should lift the declaration, but without an IR, it's hard to find a hoisting point in the general case

function main()
    local c::Bool = true

    if c
        x = 1
    elseif !c
        x = 2
    else
        x = 3
    end

    y = x # ERROR(missing): local
end
