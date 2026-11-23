function main()
    local x::Float32 = 1.5f0

    x > 0.0f0 && (x = 0.0f0) # ERROR: type mismatch
    x > 0.0f0 || (x = 0.0f0) # ERROR: type mismatch
    x > 0.0f0 && return      # ERROR(missing): type mismatch
end
