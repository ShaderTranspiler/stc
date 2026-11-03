@gl_layout std430 binding=0 @gl_buffer struct Data
    x::Float32
end

# COMMENT: same as shadowing a regular global

function main()
    x = 1.5f0 # RE OUTPUT: float x_\d+ = 1\.5
end
