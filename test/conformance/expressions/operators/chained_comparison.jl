# COMMENT: written by Claude, reviewed by szgerii

# TODO: this needs IR to handle correctly (due to short-circuit single-eval guarantees)
# TODO: we should still print a nicer error message than the catch-all Expr rejection one

function main()
    local x::Int32 = 1

    # ERROR(+1, missing): chained comparison
    res1 = 0 < x < 2      # ERROR: unsupported Expr node
    # ERROR(+1, missing): chained comparison
    res2 = 0 < x <= x < 2 # ERROR: unsupported Expr node
end
