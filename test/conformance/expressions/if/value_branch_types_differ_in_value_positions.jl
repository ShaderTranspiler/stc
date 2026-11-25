# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: the branches have to agree on a type wherever the if is used as a value, not just as an assignment's value

g(x::Int32) = x

# COMMENT: valid on its own: the trailing if's value is never used, so the method returns nothing; using it as a value fails below
# COMMENT: this is mostly because for now, value if-s with mismatched branch types infer Nothing for safety (this can never be materialized)
h(c::Bool) = c ? 1 : 1.5f0

function main()
    local c::Bool = true
    local d::Bool = false

    a = g(c ? 1 : 1.5f0)        # ERROR
    b = (c ? 1 : 1.5f0) + 1     # ERROR
    e = c ? (d ? 1 : 1.5f0) : 2 # ERROR
    r = h(c)                    # ERROR

    k = if c # ERROR
        1
    elseif d
        2
    else
        true
    end
end
