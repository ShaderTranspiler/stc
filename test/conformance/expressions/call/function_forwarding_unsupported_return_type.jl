# COMMENT: written by Claude, reviewed by szgerii

# STC FLAG(--fwd-fns)

function main()
    local x::Float32 = 1.5f0

    res = string(x) # ERROR: unsupported return type
end
