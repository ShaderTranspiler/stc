# RE OUTPUT: void f1_1_\d+\(\)
function f1_1()
end

# RE OUTPUT: void f1_2_\d+\(\)
function f1_2()::Nothing
end

# RE OUTPUT: void f1_3_\d+\(\)
function f1_3()::Nothing
    return
end

# RE OUTPUT: void f1_4_\d+\(\)
function f1_4()
    return
end

f1_5() = nothing

# RE OUTPUT: void f1_6_\d+\(\)
function f1_6()
    return nothing
end
