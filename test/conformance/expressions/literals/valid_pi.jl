# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): pi

function main()
    res1 = pi          # RE OUTPUT: float res1_\d+ = 3\.14159
    res2 = π           # RE OUTPUT: float res2_\d+ = 3\.14159
    res3 = Float32(pi) # OUTPUT: float res3
    res4 = 2f0 * pi    # OUTPUT: float res4
end
