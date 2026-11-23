# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: std430 pads each vec3 element to 16 bytes while Julia packs Vec3F into 12, matching the buffer's memory layout is the user's responsibility
# COMMENT: this aligns with how we handle other GL-specific features (code gen is our job, keeping general OpenGL stuff correctly working is theirs)

@gl_layout std430 binding=0 @gl_buffer struct Data
    values::Vector{Vec3F} # OUTPUT: vec3 values[];
end

function main()
    v = values[1] # OUTPUT: vec3 v
    values[2] = v
end
