# COMMENT: written by Claude, reviewed by szgerii

f(x::Int32) = x

function main()
    f(1; y = 2) # ERROR: Keyword arguments
    f(x = 1)    # ERROR: Keyword arguments
end
