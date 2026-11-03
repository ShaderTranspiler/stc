struct S
    x::Float32
end

struct S # ERROR: multiple definitions
    x::Int32
end
