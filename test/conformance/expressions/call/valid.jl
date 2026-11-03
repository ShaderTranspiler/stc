f() = nothing
g(x::Int32) = x
h(x::Float32, y::Float32) = x + y

function main()
    f()

    # RE OUTPUT: int res1_\d+
    res1 = g(1)

    # RE OUTPUT: float res2_\d+
    res2 = h(1.5f0, 0.5f0)
end
