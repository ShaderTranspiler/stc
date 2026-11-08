# COMMENT: written by Claude, reviewed by szgerii

struct P
    x::Float32
    y::Float32
end

@gl_layout std430 binding=0 @gl_buffer struct Data
    p::P # OUTPUT: P p;
    values::Vector{Float32}
end

function main()
    x = p.x
    values[1] = p.y
end
