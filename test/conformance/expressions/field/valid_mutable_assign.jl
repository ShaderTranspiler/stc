mutable struct Mutable
    x::Float32
    i::Int32
end

function main()
    m = Mutable(2.5f0, 1)

    m.x = 0.25f0 # RE OUTPUT: \(m_\d+\)\.x =
    m.i += 2     # RE OUTPUT: \(m_\d+\)\.i \+=

    # COMMENT: Julia converts these to the field type
    m.x = 3
end
