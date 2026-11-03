f(x::Int32 = 1) = nothing # ERROR: default initialized parameters are currently not supported

g(x = 1) = nothing # ERROR: default initialized parameters are currently not supported
