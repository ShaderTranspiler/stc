function f(x::Bool = true) # ERROR: default initialized parameter
end

function g(x::Int32, y::Bool = true) # ERROR: default initialized parameter
end

function h(x = true) # ERROR: default initialized parameter
end
