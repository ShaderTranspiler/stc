# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0

    sin()         # ERROR: overload
    sin(x, x)     # ERROR: overload
    clamp(x, 0f0) # ERROR: overload
end
