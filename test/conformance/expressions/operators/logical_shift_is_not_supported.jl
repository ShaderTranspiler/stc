function main()
    local i::Int32 = 1
    local u::UInt32 = 2

    i >>> 2 # ERROR(missing): >>>
    u >>> 2 # ERROR(missing): >>>

    i >>>= 2 # ERROR: >>>
    u >>>= 2 # ERROR: >>>
end
