function main()
    if 2 # ERROR: type mismatch
    end

    if 1.5f0        # ERROR: type mismatch
    elseif 0        # ERROR: type mismatch
    elseif Vec3F(0) # ERROR: type mismatch
    end
end
