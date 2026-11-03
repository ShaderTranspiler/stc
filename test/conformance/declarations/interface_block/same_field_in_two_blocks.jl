@gl_buffer struct Data1
    x::Float32
end

@gl_buffer struct Data2
    x::Float32 # ERROR: redefinition of name 'x'
end
