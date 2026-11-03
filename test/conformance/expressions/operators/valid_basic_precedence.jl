function main()
    # COMMENT: uses literals to avoid OUTPUT check depending heavily on name mangling rules
    # COMMENT: also note that the transpiler uses the strictest paren placement possible (hence the parens around literals)

    0 + 1 * 2 * 3 - 4 # OUTPUT: ((0) + (((1) * (2)) * (3))) - (4)
end
