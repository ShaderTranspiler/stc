# COMMENT: written by Claude, reviewed by szgerii

function main()
    # ERROR(+1, missing): non-materializable
    local x::Int64 = 1 # ERROR: i64
end
