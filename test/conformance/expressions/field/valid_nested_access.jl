# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): dot chain

struct Inner
    x::Float32
end

struct Outer
    inner::Inner
    v::Vec3F
end

function main()
    o = Outer(Inner(1.5f0), Vec3F(0))

    x = o.inner.x # RE OUTPUT: float x_\d+ =
    z = o.v.z     # RE OUTPUT: float z_\d+ =
end
