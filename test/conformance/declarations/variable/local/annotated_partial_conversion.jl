# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: Julia throws an InexactError for every non-integral value here, which GLSL would silently truncate, so float to integer conversions aren't implicit (the explicit Int32(x) is accepted with a warning)

function main()
    local x::Float32 = 1.5f0

    local a::Int32  = x        # ERROR: type mismatch
    local b::UInt32 = x        # ERROR: type mismatch
    local c::IVec3  = Vec3F(1) # ERROR: type mismatch
end
