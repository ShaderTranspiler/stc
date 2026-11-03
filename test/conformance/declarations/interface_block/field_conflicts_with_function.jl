function x()
end

@gl_buffer struct Data
    x::Float32 # ERROR: redefinition of name 'x'
end
