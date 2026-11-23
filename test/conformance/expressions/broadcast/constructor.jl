# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid in Julia, but not supported
# COMMENT: Float32.(v) would produce a Vec3F with each component of v converted to Float32
# COMMENT: Vec3F.(x) is virtually the same as Vec3F(x) [all elements set to x]

function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1)

    res1 = Float32.(v) # ERROR: broadcast call to constructor
    res2 = Vec3F.(x)   # ERROR: broadcast call to constructor
end
