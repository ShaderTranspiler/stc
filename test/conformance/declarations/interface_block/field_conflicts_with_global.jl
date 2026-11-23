@gl_uniform global x::Float32

@gl_buffer struct Data
    x::Float32 # ERROR: redefinition of name 'x'
end
