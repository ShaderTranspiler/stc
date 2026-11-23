# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: length of a number is 1 in Julia, but GLSL's length on a float is its absolute value

function main()
    local x::Float32 = -1.5f0

    res = length(x) # ERROR(missing)
end
