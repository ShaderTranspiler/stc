mutable struct Mutable
    i::Int32
end

function main()
    m = Mutable(1)

    m.i = 1.5f0    # ERROR: type mismatch
    m.i = Vec3F(0) # ERROR: type mismatch
end
