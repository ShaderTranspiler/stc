# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: Julia resolves parameter types when the method is defined, so S has to be defined before f
# COMMENT: weirdly, this does not match when it resolves the return type, or any of the body, so this differs from those cases in handling

# ERROR(+1, missing): undefined
f(s::S) = s.x # ERROR: S

struct S
    x::Float32
end
