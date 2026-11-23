# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: in Julia, the value of an assignment is its right-hand side before conversion, so a is an Int32 even though b is a Float32

function main()
    local b::Float32

    a = b = 1 # RE OUTPUT(missing): int a_\d+
end
