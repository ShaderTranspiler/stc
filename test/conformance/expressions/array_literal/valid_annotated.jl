# COMMENT: written by Claude, reviewed by szgerii

function main()
    local a::Vector{Float32} = [1.5f0, 2.5f0] # RE OUTPUT: float a_\d+\[2\] = float\[2\]\(
    local b::Vector{Int32} = [1, 2]           # RE OUTPUT: int b_\d+\[2\] = int\[2\]\(1, 2\);
end
