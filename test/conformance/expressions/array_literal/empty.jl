function main()
    a = [] # ERROR: empty array literal

    # COMMENT: valid in Julia (Float32[]), but GLSL has no zero-length arrays
    local b::Vector{Float32} = [] # ERROR(missing): empty array literal
end
