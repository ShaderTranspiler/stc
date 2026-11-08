# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

struct S
    x::Float32
end

global g_x::Float32 = 1.5f0

g(x::Float32) = x

function main()
    res1 = Main.g(1.5f0) # RE OUTPUT: float res1_\d+ = g_\d+\(
    res2 = Main.S(1.5f0) # OUTPUT: S res2
    res3 = Main.g_x      # RE OUTPUT: float res3_\d+ = g_x
end
