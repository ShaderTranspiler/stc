# COMMENT: written by Claude, reviewed by szgerii

function main()
    a = [] # ERROR: empty array literal

    # COMMENT: valid in Julia (Float32[]), but GLSL has no zero-length arrays
    local b::Vector{Float32} = [] # ERROR: empty array literal
end
