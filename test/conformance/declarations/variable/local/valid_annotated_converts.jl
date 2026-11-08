# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0
    local i::Int32   = 1
    local u::UInt32  = UInt32(5)

    local a::UInt32  = 5    # OUTPUT: uint a
    local b::Float32 = true # OUTPUT: float b
    local c::Float32 = u    # OUTPUT: float c
    local d::Float32 = i    # OUTPUT: float d
    local e::Float64 = x    # OUTPUT: double e
end
