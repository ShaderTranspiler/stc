# COMMENT: written by Claude, reviewed by szgerii

@gl_layout std430 binding=0 @gl_buffer struct Data
    x::Float32
end

function main()
    global x
    x = 1.5f0 # RE OUTPUT(multiline): ^x = 1\.5
end
