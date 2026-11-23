function main()
    x = Int32(0)
    # COMMENT: Julia changes x's type to Float32 here (because the declaration is unannotated)
    # ERROR(+1)
    x += 1.5f0
end
