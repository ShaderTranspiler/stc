# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: tuples are only supported on both sides of multiple assignments (a, b = 1, 2), not as values (test in statements/assignment)
# COMMENT: maybe in the future we can do automatic vecN conversion of (1/)2/3/4-component tuples of scalars, rejected for now

function f()::Int32
    return 1, 2 # ERROR: unsupported Expr node
end

function main()
    t1 = (1, 2)         # ERROR: unsupported Expr node
    t2 = (a = 1, b = 2) # ERROR: unsupported Expr node
end
