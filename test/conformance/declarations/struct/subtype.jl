# COMMENT: written by Claude, reviewed by szgerii

struct S <: Number # ERROR: composite subtypes
    x::Float32
end
