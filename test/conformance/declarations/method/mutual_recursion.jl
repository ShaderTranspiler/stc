function f()::Int32
    return g() # ERROR(missing): mutual recursion
end

function g()::Int32
    return f() # ERROR(missing): mutual recursion
end
