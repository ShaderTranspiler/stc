# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid in Julia, but GLSL builtins don't take arrays

function main()
    local a::Vector{Float32} = [1.5f0, 2.5f0]

    res = sin.(a) # ERROR: overload
end
