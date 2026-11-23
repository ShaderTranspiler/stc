# COMMENT: written by Claude, reviewed by szgerii

function main()
    a = [1, 2, 3]
    i = 2

    # COMMENT: a[:2] would be the second element, but only works with literals (text after : is treated as a symbol literal)
    c = a[:i] # ERROR: non-integer indexer
end
