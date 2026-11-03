struct S
    b::Bool
    i::Int32
    u::UInt32
    f::Float32
    d::Float64
    v::Vec3F
    iv::IVec2
end

function main()
    s = S(true, 1, 2, 1.5f0, 1.5, Vec3F(0), IVec2(1))

    b = s.b   # RE OUTPUT: bool b_\d+ =
    i = s.i   # RE OUTPUT: int i_\d+ =
    u = s.u   # RE OUTPUT: uint u_\d+ =
    f = s.f   # RE OUTPUT: float f_\d+ =
    d = s.d   # RE OUTPUT: double d_\d+ =
    v = s.v   # RE OUTPUT: vec3 v_\d+ =
    iv = s.iv # RE OUTPUT: ivec2 iv_\d+ =
end
