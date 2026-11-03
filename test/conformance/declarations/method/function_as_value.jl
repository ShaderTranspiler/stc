f() = nothing

function g() # INTERNAL ERROR: method reached its own declaration
    g # ERROR(missing)
    f # ERROR(missing)
end

