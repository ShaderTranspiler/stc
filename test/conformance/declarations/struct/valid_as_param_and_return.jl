struct P
    x::Float32
    y::Float32
end

# RE OUTPUT: P swapped_\d+\(P p_\d+\)
function swapped(p::P)::P
    return P(p.y, p.x)
end

function main()
    # RE OUTPUT: P p_\d+ =
    p = swapped(P(1.5f0, 2.5f0))
end
