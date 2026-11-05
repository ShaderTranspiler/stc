# STC(missing): cannot assign bool to f32

mutable struct Mutable
    x::Float32
end

function main()
    m = Mutable(1.5f0)

    m.x = true
end
