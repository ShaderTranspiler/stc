# STC(missing): reassignment of f32 to bool

mutable struct Mutable
    x::Float32
end

function main()
    m = Mutable(1.5f0)

    m.x = true
end
