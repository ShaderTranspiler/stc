function assign_1(x::Int32)
    x = 1.5f0 # ERROR: type mismatch
end

function assign_2(x::Float32)
    x = Int32(1) # ERROR(missing): type mismatch
    return x
end

function decl_1(x::Int32)
    local x::Float32 # ERROR: redeclared
end

function decl_2(x::Int32)
    local x::Float32 = 1.5f0 # ERROR: redeclared
end
