# COMMENT: this is a case where Julia would throw an InexactError when RHS is not convertible to Int32 (1.5f0 instead of 2.0f0), but safety cannot be decided at the type level, so we reject it explicitly for every case

function main()
    local x::Int32 = 0
    x = Float32(2) # ERROR: type mismatch
end
