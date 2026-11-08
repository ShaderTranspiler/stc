# COMMENT: written by Claude, reviewed by szgerii

# SKIP: stc throws an uncaught exception while lowering the global declared after its first access

# COMMENT: valid in Julia, since f only runs after g has been assigned

function f()::Float32
    return g
end

global g::Float32 = 1.5f0

function main()
    x = f() # OUTPUT: float x
end
