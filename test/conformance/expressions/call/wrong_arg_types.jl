f(x::Int32) = nothing

function main()
    f(1.5f0)    # ERROR: no method
    f(true)     # ERROR: no method
    f(Vec3F(0)) # ERROR: no method
end
