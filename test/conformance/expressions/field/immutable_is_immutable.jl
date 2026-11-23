struct Immutable
    x::Float32
    i::Int32
end

function main()
    im = Immutable(2.5f0, 1)
    im.x = 0.5f0 # ERROR: immutable
    im.i = 2     # ERROR: immutable
end
