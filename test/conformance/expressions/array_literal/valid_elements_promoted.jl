# STC(missing): varying element type
# COMMENT: Julia promotes mixed element types (Bool < Int32 < UInt32 < Float32 < Float64), the order of the elements should not change the resulting type

function main()
    local x::Float32 = 1.5f0
    local d::Float64 = 1.5
    local i::Int32 = 2
    local u::UInt32 = 3
    local b::Bool = true

    a1 = [x, i] # RE OUTPUT: float a1_\d+\[2\] = float\[2\]\(
    a2 = [i, x] # RE OUTPUT: float a2_\d+\[2\] = float\[2\]\(
    a3 = [b, x] # RE OUTPUT: float a3_\d+\[2\] = float\[2\]\(
    a4 = [b, i] # RE OUTPUT: int a4_\d+\[2\] = int\[2\]\(
    a5 = [i, u] # RE OUTPUT: uint a5_\d+\[2\] = uint\[2\]\(
    a6 = [i, d] # RE OUTPUT: double a6_\d+\[2\] = double\[2\]\(

    # COMMENT: literals go through the same promotion
    a7 = [2, 1.5f0] # RE OUTPUT: float a7_\d+\[2\] = float\[2\]\(
end
