# COMMENT: written by Claude, reviewed by szgerii

function main()
    struct S # ERROR: non-global scope
        x::Float32
    end
end
