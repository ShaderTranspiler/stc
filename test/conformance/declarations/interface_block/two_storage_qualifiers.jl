# COMMENT: written by Claude, reviewed by szgerii

@gl_buffer @gl_uniform struct Data # ERROR: more than one storage qualifier
    x::Float32
end
