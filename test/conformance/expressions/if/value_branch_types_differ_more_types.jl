# COMMENT: written by Claude, reviewed by szgerii

function main()
    local c::Bool = true
    local i::Int32 = 1
    local u::UInt32 = UInt32(1)

    a = c ? i : u                  # ERROR
    b = c ? Vec3(1.0f0) : IVec3(1) # ERROR
    d = c ? 1 : nothing            # ERROR
end
