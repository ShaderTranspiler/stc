# COMMENT: written by Claude, reviewed by szgerii

# TODO: STC FLAG(-Wstrict-conformance)
# COMMENT: Julia throws a DomainError outside these functions' domains, GLSL leaves the result undefined, so we warn under -Wstrict-conformance

function main()
    local x::Float32 = 0.5f0

    sqrt(x)  # WARNING(missing)
    log(x)   # WARNING(missing)
    log2(x)  # WARNING(missing)
    asin(x)  # WARNING(missing)
    acos(x)  # WARNING(missing)
    acosh(x) # WARNING(missing)
    atanh(x) # WARNING(missing)
end
