# COMMENT: written by Claude, reviewed by szgerii

@gl_layout std430=1 @gl_buffer struct Data # ERROR: does not expect a value
    x::Float32
end
