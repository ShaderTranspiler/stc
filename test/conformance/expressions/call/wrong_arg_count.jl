f(x::Int32) = nothing

function main()
    f()     # ERROR: no method
    f(1, 2) # ERROR: no method
end
