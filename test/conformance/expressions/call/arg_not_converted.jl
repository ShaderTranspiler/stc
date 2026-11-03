f(x::Float32) = nothing

function main()
    f(1) # ERROR: no method
end
