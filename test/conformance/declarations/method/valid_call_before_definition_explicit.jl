# GLSL(missing): no matching overloaded function

# RE OUTPUT: float f_\d+\(\)
function f()::Float32
    g()
end

function g()::Float32
    1.5f0
end
