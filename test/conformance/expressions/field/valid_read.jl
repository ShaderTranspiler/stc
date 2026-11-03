struct S
    x::Float32
    v::Vec3F
end

function main()
    s = S(1.5f0, Vec3F(0))

    x = s.x # RE OUTPUT: float x_\d+ = \(s_\d+\)\.x;
    v = s.v # RE OUTPUT: vec3 v_\d+ = \(s_\d+\)\.v;
end
