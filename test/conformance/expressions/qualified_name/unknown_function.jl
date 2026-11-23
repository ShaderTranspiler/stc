# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0
    Base.nonexistent_function(x) # ERROR: no function with name
end
