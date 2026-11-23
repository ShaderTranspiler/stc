# COMMENT: written by Claude, reviewed by szgerii

# TODO: STC FLAG(-Wstrict-conformance)
# STC(missing): ^

# COMMENT: a runtime exponent lowers to pow, which GLSL leaves undefined for negative bases (Julia throws a DomainError) and for a zero base with a non-positive exponent (Julia gives Inf or 1)

function main()
    local x::Float32 = 1.5f0
    local y::Float32 = 2.5f0

    res = x ^ y # WARNING(missing)
end
