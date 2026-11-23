# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

# COMMENT: unary '-' binds weaker than '^' in Julia, so -x^2 == -(x^2)

function main()
    local x::Int32 = 2
    res = -x^2 # RE OUTPUT: int res_\d+ = -\(\(x_\d+\) \* \(x_\d+\)\);
end
