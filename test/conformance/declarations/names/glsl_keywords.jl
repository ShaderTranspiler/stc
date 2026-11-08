# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: these names are emitted without mangling, so GLSL keywords and reserved words can't be used for them

struct sample # ERROR(missing)
    x::Float32
end

struct vec2 # ERROR(missing)
    x::Float32
end

struct S
    sample::Float32 # ERROR(missing)
    input::Float32  # ERROR(missing)
end

@gl_buffer struct output # ERROR(missing)
    x::Float32
end

@gl_buffer struct Data
    filter::Float32 # ERROR(missing)
end

@gl_uniform global input::Float32 # ERROR(missing)
