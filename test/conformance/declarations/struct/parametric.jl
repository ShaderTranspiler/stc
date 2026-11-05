# COMMENT: written by Claude, reviewed by szgerii

struct S{T} # ERROR: parametric composite types
    x::T
end
