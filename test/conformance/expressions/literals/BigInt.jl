# COMMENT: written by Claude, reviewed by szgerii

function main()
    # COMMENT: literals larger than typemax(Int128) get a macro representation in the AST
    # ERROR(+1, internal): macrocall
    1000000000000000000000000000000000000000000 # ERROR(missing): integer literal too large
end
