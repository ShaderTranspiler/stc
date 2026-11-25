# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid Julia, but would have to take into account both the last expression of the method body (the if) as the return value implicitly, and also the return in one of its subexpressions
# COMMENT: the transpiler can't handle this for now, since it needs to check a lot of stuff to ensure type stability here, across branches, early returns and the "accumulated" type for the if expr

function f(c::Bool)::Int32
    if c return 1 else 2 end # ERROR
end
