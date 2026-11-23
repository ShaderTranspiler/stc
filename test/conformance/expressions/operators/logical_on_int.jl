function main()
    local x::Int32 = 0
    !x        # ERROR(missing): type mismatch
    x && true # ERROR: type mismatch
    x || true # ERROR: type mismatch
end
