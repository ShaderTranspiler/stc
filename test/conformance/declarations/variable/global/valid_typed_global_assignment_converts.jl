# COMMENT: written by Claude, reviewed by szgerii

global g::Float32 = 0f0

# COMMENT: g has a declared type, so assigning a value of another type converts it

function main()
    global g
    g = 1
end
