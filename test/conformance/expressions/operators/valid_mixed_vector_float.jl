function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1)

    # COMMENT: the form 'x op v op x' also checks commutativity (assuming the partial result remains a vec3, validated by the OUTPUT checks indirectly under most reasonable conditions)

    # COMMENT: + and - need the dot for element-wise application in JuliaGLM, and scalar / vector is not element-wise in Julia
    res3 = x * v * x # OUTPUT: vec3 res3
    res4 = v / x     # OUTPUT: vec3 res4

    res5 = x .+ v .+ x # OUTPUT: vec3 res5
    res6 = x .- v .- x # OUTPUT: vec3 res6
    res7 = x .* v .* x # OUTPUT: vec3 res7
    res8 = x ./ v ./ x # OUTPUT: vec3 res8
end
