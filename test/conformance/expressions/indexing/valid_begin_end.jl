# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): end

function main()
    local a::Vector{Float32} = [1.5f0, 2.5f0, 3.5f0]
    local v::Vec3F = Vec3F(1)

    res1 = a[end]   # OUTPUT: float res1
    res2 = a[begin] # OUTPUT: float res2
    res3 = v[end]   # OUTPUT: float res3
    res4 = v[begin] # OUTPUT: float res4

    a[begin] = 0f0
    a[end] = 0f0
end
