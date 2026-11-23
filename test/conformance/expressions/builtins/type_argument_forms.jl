# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0

    # ERROR(+1, missing): round
    res1 = round(Int32, x) # ERROR: Int32
    # ERROR(+1, missing): floor
    res2 = floor(Int32, x) # ERROR: Int32
end
