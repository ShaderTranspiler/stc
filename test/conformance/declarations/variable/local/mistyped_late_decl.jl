function main()
    x = 1.5f0 # ERROR(missing): type mismatch
    # TODO: remove this once the error moves to the assignment's location
    local x::Int32 # ERROR: redeclaration
end
