# COMMENT: written by Claude, reviewed by szgerii

function main()
    # COMMENT: literals larger than typemax(Int64) get a macro representation in the AST
    # ERROR(+1, internal): macrocall
    170141183460469231731687303715884105727 # ERROR(missing): integer literal too large
end
