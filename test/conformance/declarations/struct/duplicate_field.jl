# COMMENT: written by Claude, reviewed by szgerii

struct S
    x::Float32
    x::Int32 # ERROR: repeated field name
end
