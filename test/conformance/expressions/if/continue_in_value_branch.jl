# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid Julia, but the continue would end up inside an operand of the generated ternary in GLSL

function main()
    local i::Int32 = 0

    while i < 10
        i += 1
        x = i == 2 ? (continue) : i # ERROR
    end
end
