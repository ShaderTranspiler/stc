f1(x::Int32 = 1) = nothing # ERROR: default initialized parameters are currently not supported

f2(x = 1) = nothing # ERROR: default initialized parameters are currently not supported

function f3(x::Bool = true) # ERROR: default initialized parameter
end

function f4(x::Int32, y::Bool = true) # ERROR: default initialized parameter
end

function f5(x = true) # ERROR: default initialized parameter
end

