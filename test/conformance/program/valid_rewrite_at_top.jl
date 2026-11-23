# COMMENT: these should get inlined into the "global compound" as global variables

# RE OUTPUT: int a_\d+ =
# RE OUTPUT: float b_\d+ =
# RE OUTPUT: bool c_\d+ =
a, b, c = 1, 2.0f0, true

# COMMENT: check that they're accessible from any function (i.e. actually global)
function f()::Nothing
    x = a
    y = b
    z = c
    
    return nothing
end

function main()
    x = a
    y = b
    z = c
end

