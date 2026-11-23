# COMMENT: written by Claude, reviewed by szgerii

# SKIP: stc hits a debug assertion when an array literal is checked against a non-array type

function f()::Float32
    return [1.5f0] # ERROR: type mismatch
end

function main()
    local v::Vec3F = [1.5f0, 2.5f0, 3.5f0] # ERROR: type mismatch
    local x::Float32 = [1.5f0]             # ERROR: type mismatch
end
