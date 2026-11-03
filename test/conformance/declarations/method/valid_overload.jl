function f(x::Int32)::Bool
    return true
end

function f(x::Float32)::Float32
    return 1.5f0
end

function main()
    x = f(0) # RE OUTPUT: bool x_\d+ =
    y = f(0.0f0) # RE OUTPUT: float y_\d+ =
end
