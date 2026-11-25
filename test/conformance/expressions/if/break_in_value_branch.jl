# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid Julia, but the break would end up inside an operand of the generated ternary in GLSL

function main()
    local i::Int32 = 0

    while true
        i += 1
        x = i > 3 ? (break) : i # ERROR
    end
end
