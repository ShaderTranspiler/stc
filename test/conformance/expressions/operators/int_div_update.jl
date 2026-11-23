# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: / on integers gives a float in Julia, which can't be stored back into the integer target

function main()
    local x::Int32 = 6
    local y::Int32 = 4
    local v::IVec3 = IVec3(6)
    local w::IVec3 = IVec3(4)

    x /= y # ERROR(missing)
    v /= w # ERROR
end
