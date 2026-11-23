f() = 1
f() = 2 # ERROR: signature

g(x::Int32) = 1
g(x::Int32) = 2 # ERROR: signature

function h(x::Int32)::Float32
    1
end

function h(x::Int32)::Float32 # ERROR: signature
    2
end
