# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: builtin function names aren't reserved for structs, and GLSL only reserves __ by convention (defining such names is not a GLSL compiler error)

struct mix
    x::Float32
end

struct S__
    x_::Float32
end

function main()
    a__b = 1.5f0
    c_ = 2.5f0
    m = mix(1.5f0)
end
