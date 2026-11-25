# COMMENT: written by Claude, reviewed by szgerii

function main()
    local c::Bool = true

    x = c ? begin begin y = 1; y + 1 end end : 3 # ERROR
end
