# COMMENT: written by Claude, reviewed by szgerii

global S::Float32 = 0.0f0

struct S # ERROR: 'S'
    x::Float32
end
