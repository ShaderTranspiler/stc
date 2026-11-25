# OUTPUT: void f_
f() = return nothing

# OUTPUT: void g_
function g()::Nothing
    return nothing
end

# OUTPUT: void h_
function h()
    2
    nothing
end

# RE OUTPUT: void i_
function i()
    return f()
end

# RE OUTPUT: void j_
function j(some_bool::Bool)
    return some_bool ? f() : g()
end
