# COMMENT: written by Claude, reviewed by szgerii

# TODO: STC FLAG(-Wstrict-conformance)

# COMMENT: Julia throws an InexactError only for values out of the target's range, where GLSL reinterprets the bits instead, so these convert implicitly with a warning (like the explicit UInt32(i) and Int32(u))

function main()
    local i::Int32 = 1
    local u::UInt32 = UInt32(1)

    local a::UInt32 = i          # WARNING(missing)
    local b::Int32  = u          # WARNING(missing)
    local c::UInt32 = -1         # WARNING(missing)
    local d::Int32  = 0xffffffff # WARNING(missing)
end
