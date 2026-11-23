# COMMENT: written by Claude, reviewed by szgerii

global x::Float32 = 1.5f0

# COMMENT: the assignment makes x a local in the whole method body, not just after it, so the read before it doesn't see the global

function f()
    y = x # ERROR: undeclared
    x = 2.5f0
end

function main()
    f()
end
