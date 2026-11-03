function main()
    # COMMENT: Julia promotes both of these to Vector{Float32}

    # RE OUTPUT: float a_\d+\[2\] = float\[2\]\(
    a = [1.5f0, 2]

    # RE OUTPUT: float b_\d+\[2\] = float\[2\]\(
    local b::Vector{Float32} = [1, 2]

    local x::Float32 = 1.5f0
    local i::Int32 = 2

    # RE OUTPUT: float c_\d+\[2\] = float\[2\]\(
    local c::Vector{Float32} = [x, i]

    # RE OUTPUT: float d_\d+\[2\] = float\[2\]\(
    local d::Vector{Float32} = [i, x]
end
