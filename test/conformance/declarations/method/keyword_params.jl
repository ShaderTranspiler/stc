# COMMENT: written by Claude, reviewed by szgerii

f(x::Int32; y::Int32) = x + y     # ERROR: kwargs
g(x::Int32; y::Int32 = 1) = x + y # ERROR: kwargs
h(x::Int32; y::Int32 = 1, z::Float32) = x + y + z # ERROR: kwargs

function h(; y::Int32) # ERROR: kwargs
end
