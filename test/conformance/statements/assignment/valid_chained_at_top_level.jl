# COMMENT: written by Claude, reviewed by szgerii

# GLSL(missing): unexpected IDENTIFIER

# RE OUTPUT(multiline): ^(\s*)float a
# RE OUTPUT(multiline, missing): ^(\s*)float b

a = b = 1.5f0

function main()
    # OUTPUT: float c
    c = a + b
end
