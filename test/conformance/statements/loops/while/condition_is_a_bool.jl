# STC FLAG(--no-coerce-f32)

function main()
    while 2       end # ERROR: type mismatch
    while 0x00002 end # ERROR: type mismatch
    while 2.5f0   end # ERROR: type mismatch
    while 2.5     end # ERROR: type mismatch
end
