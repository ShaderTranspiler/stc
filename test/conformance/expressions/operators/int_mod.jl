# COMMENT: for now, we only lower integer modulo for unsigned types (signed types require signedness match, otherwise Julia and GLSL behavior drifts)
# COMMENT: could be an inserted helper later

function main()
    local x::Int32 = 1
    local y::Int32 = 2
    local u::UInt32 = 3
    mod(x, y) # ERROR(missing): signed
    mod(x, u) # ERROR(missing): signed
    mod(u, x) # ERROR(missing): signed
end
