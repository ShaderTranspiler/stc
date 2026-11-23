# COMMENT: written by Claude, reviewed by szgerii

struct S
    x::Float32
    S() = new(0.0f0)       # ERROR: inner constructor
    S(x::Float32) = new(x) # ERROR: inner constructor
end
