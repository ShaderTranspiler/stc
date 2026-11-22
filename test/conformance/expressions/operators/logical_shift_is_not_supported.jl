function main()
    local i::Int32 = 1
    local u::UInt32 = 2

    i >>> 2 # ERROR: >>>
    u >>> 2 # ERROR: >>>

    i >>>= 2 # ERROR: >>>
    u >>>= 2 # ERROR: >>>
end
