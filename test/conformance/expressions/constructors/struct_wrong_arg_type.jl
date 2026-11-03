struct S
    i::Int32
end

function main()
    s1 = S(1.5f0)    # ERROR: type mismatch
    s2 = S(Vec3F(0)) # ERROR: type mismatch
end
