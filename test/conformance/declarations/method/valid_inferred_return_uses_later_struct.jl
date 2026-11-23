# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): couldn't find function

# COMMENT: Julia only requires the struct to be defined by the time g is called, not by its definition

g() = S(1.5f0)

struct S
    x::Float32
end

function main()
    g()
end
