# COMMENT: written by Claude, reviewed by szgerii

# GLSL(missing): unexpected IDENTIFIER

function main()
    # OUTPUT: int a
    # OUTPUT: int b
    # OUTPUT: int c
    a = b = c = 1

    local x::Int32 = 0
    local y::Int32 = 0
    local z::Int32 = 0
    x = y = z = 2
end
