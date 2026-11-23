# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): non-scalar element types

struct S
    x::Float32
end

function main()
    a = [Vec3F(0), Vec3F(1)]                       # RE OUTPUT: vec3 a_\d+\[2\] = vec3\[2\]\(
    b = [S(1.5f0), S(2.5f0)]                       # RE OUTPUT: S b_\d+\[2\] = S\[2\]\(
    local c::Vector{IVec2} = [IVec2(1), IVec2(2)] # RE OUTPUT: ivec2 c_\d+\[2\] = ivec2\[2\]\(

    x = a[1]   # OUTPUT: vec3 x
    y = a[2].x # OUTPUT: float y
    z = b[1].x # OUTPUT: float z
end
