f(x::Nothing) = nothing # ERROR: non-materializable
f(x::String) = nothing  # ERROR: non-materializable
f(x::Symbol) = nothing  # ERROR: non-materializable
