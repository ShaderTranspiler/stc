# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: unlike ==, === tells apart signed zeros and treats NaN as equal to itself, reject it for now

function main()
    local x::Float32 = 1.5f0

    res1 = x === 2f0 # ERROR: ===
    res2 = x !== 2f0 # ERROR: !==
end
