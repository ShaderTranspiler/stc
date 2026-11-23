f(x::Int32, x::Float32) = nothing # ERROR: 'x'
g(x::Int32, x::Int32) = nothing   # ERROR: 'x'

# ERROR(+2): 'x'
# ERROR(+1): 'y'
h(x::Int32, y::Int32, x::Float32, y::Float32) = nothing
