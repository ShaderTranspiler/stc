# COMMENT: written by Claude, reviewed by szgerii

mutable struct Mutable
    x::Float32
end

function main()
    m = Mutable(1.5f0)

    y = m.y     # ERROR: has no field with identifier 'y'
    m.y = 2.5f0 # ERROR: has no field with identifier 'y'
end
