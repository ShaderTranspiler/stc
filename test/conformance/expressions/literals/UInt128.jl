function main()
    # COMMENT: literals larger than typemax(UInt64) get a macro representation in the AST
    # ERROR(+1, internal): macrocall
    0xffffffffffffffff1 # ERROR(missing): integer literal too large
end
