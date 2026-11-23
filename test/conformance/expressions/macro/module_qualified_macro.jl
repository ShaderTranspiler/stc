# COMMENT: written by Claude, reviewed by szgerii

# ERROR(+1, missing): non-qualifier macro
Base.@kwdef struct S # INTERNAL ERROR: unexpected macrocall layout
    x::Float32 = 1.5f0
end
