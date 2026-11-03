# RE OUTPUT: void f1_1_\d+\(\)
function f1_1()
end

function f2(x::Int32)::Int32
    return x + 1
end

# RE OUTPUT: float f3_1_\d+\(
function f3_1(x::Float32, y::Float32)::Float32
    return x + y
end

# RE OUTPUT: float f3_2_\d+\(
function f3_2(x::Float32, y::Float32)
    return x + y
end

# RE OUTPUT: float f3_3_\d+\(
function f3_3(x::Float32, y::Float32)
    x + y
end

# RE OUTPUT: float f3_4_\d+\(
f3_4(x::Float32, y::Float32) = x + y

# RE OUTPUT: float f4_\d+\(
function f4()
    # COMMENT: return lifting should split assignment
    # RE OUTPUT: float f4_local_\d+ = \d+\.\d+f;
    # RE OUTPUT: return f4_local_\d+;
    f4_local = 1.5f0
end

