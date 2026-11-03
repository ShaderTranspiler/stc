function main()
    a = [1, 2]

    a[1] = 1.5f0    # ERROR: type mismatch
    a[2] = Vec3F(0) # ERROR: type mismatch
end
