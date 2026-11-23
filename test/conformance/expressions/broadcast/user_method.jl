# COMMENT: written by Claude, reviewed by szgerii

struct S
    x::Float32
end

g(x::Float32) = x + 1.5f0

function main()
    local x::Float32 = 1.5f0
    local v::Vec3F = Vec3F(1)

    res1 = g.(v) # ERROR: broadcast call
    res2 = g.(x) # ERROR: broadcast call
    res3 = S.(x) # ERROR: broadcast call
end
