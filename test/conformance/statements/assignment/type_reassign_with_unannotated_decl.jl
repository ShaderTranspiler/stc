# COMMENT: this is specifically for catching Julia's dynamic typing on reassignment that happens instead of an implicit convert when the declaration is not type annotated

function main()
    x = 0.0f0
    # COMMENT: in Julia, x becomes an Int32 here if the declaration has no annotation (the Int32 is only needed to make this independent of the transpiler's literal coercion rules)
    # ERROR(+1, missing)
    x = Int32(1)
end
