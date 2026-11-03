function main()
    local x::Int32 = 1
    local y::Int32 = 2

    local result::Int32 = 0
    result = x / y # ERROR(missing): type mismatch
end
