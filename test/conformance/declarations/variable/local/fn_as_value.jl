f() = nothing

function main()
    x = f # ERROR: non-materializable
end