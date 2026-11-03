function main()
    v = Vec3F(0)

    v[1.5f0]    # ERROR: invalid vector indexer type
    v[true]     # ERROR: invalid vector indexer type
    v[Vec2F(1)] # ERROR: invalid vector indexer type
end
