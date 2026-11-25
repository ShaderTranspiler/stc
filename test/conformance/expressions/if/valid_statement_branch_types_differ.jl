# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: as statements, the branches' values are discarded, so their types don't have to agree

f() = Int32(1)
g() = true
h() = 1.5f0

function main()
    local c::Bool = true

    c ? f() : g()

    if c
        f()
    elseif false
        h()
    else
        g()
    end
end
