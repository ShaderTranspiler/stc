# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: GLSL leaves integer division and remainder with negative operands undefined or unspecified, so they have to go through absolute values to match Julia

function main()
    local x::Int32 = -7
    local y::Int32 = 2

    res1 = div(x, y) # RE OUTPUT(missing): int res1_\d+ = .*abs\(
    res2 = x ÷ y     # RE OUTPUT(missing): int res2_\d+ = .*abs\(
    res3 = x % y     # RE OUTPUT(missing): int res3_\d+ = .*abs\(
    res4 = rem(x, y) # RE OUTPUT(missing): int res4_\d+ = .*abs\(
end
