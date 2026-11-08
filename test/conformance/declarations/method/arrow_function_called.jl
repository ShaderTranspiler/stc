# COMMENT: written by Claude, reviewed by szgerii

# SKIP: stc currently crashes on calls with a callee that failed to parse

function main()
    y = (x -> x + 1)(1) # ERROR: arrow functions
end
