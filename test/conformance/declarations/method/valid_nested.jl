function weird_identity(x::Float32)::Float32
    g(x::Float32) = x + 1.5f0

    function h(x::Float32)
        return x - 1.5f0
    end

    return h(g(x))
end
