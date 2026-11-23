# COMMENT: written by Claude, reviewed by szgerii

function main()
    local a::Int32 = 0
    local b::Float32 = 0f0

    res1 = (a = 1) + 1 # RE OUTPUT: int res1_\d+ = \(a_\d+ = 1\) \+ \(1\);

    # RE OUTPUT: if \(\(a_\d+ = 2\) > \(0\)\)
    if (a = 2) > 0
    end

    # COMMENT: known deviation: in Julia, the value of the assignment is the unconverted right-hand side (Int32 here), in GLSL it is the converted value of b
    # TODO: we could generate wrapper code to unify behavior
    res2 = (b = 1) + 1 # OUTPUT: float res2
end
