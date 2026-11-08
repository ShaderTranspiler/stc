# COMMENT: written by Claude, reviewed by szgerii

# TODO: STC FLAG(-Wstrict-conformance)
# COMMENT: Julia throws a DivideError for integer division by zero (and for div(typemin(Int32), -1)), which the GLSL operators can't catch, so every integer division warns under -Wstrict-conformance

function main()
    local i::Int32  = 7
    local j::Int32  = 2
    local u::UInt32 = UInt32(7)
    local w::UInt32 = UInt32(2)

    res1 = div(i, j)  # WARNING(missing)
    res2 = i ÷ j      # WARNING(missing)
    res3 = i % j      # WARNING(missing)
    res4 = rem(i, j)  # WARNING(missing)
    res5 = i % 2      # WARNING(missing)
    res6 = div(u, w)  # WARNING(missing)
    res7 = rem(u, w)  # WARNING(missing)
    res8 = mod(u, w)  # WARNING(missing)
end
