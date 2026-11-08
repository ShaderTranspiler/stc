# COMMENT: written by Claude, reviewed by szgerii

function main()
    v = Vec3F(0)
    a = [1.5f0, 2.5f0]

    x = v[1, 2] # ERROR: only support one indexer value
    y = a[1, 2] # ERROR: only support one indexer value
end
