# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: these should always be fine in the generated output, regardless of scoping rules and such, thanks to name mangling

f() = 1.5f0

function g()
    local f::Float32 = 3.5f0 # RE OUTPUT: float f_\d+ = 3\.5
end

function main()
    # COMMENT: assigning to the name of a global method inside a method declares a new local
    f = 2.5f0 # RE OUTPUT: float f_\d+ = 2\.5
end
