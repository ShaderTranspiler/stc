# COMMENT: written by Claude, reviewed by szgerii

# TODO: STC FLAG(-Wstrict-conformance)
# STC(missing): cannot convert

# COMMENT: conversions apply element-wise to vectors

function main()
    local v::Vec3F = IVec3(1) # OUTPUT: vec3 v
    local d::Vec3D = Vec3F(1) # OUTPUT: dvec3 d

    # COMMENT: Int32 to UInt32 throws for negative components in Julia, so it warns like the scalar case
    local uv::UVec3 = IVec3(1) # WARNING(missing)
end
