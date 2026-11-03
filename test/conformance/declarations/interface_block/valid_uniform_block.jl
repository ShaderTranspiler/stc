# OUTPUT: layout(std140, binding = 1) uniform
@gl_layout std140 binding=1 @gl_uniform struct Params
    scale::Float32
    offset::Vec3F
end

function main()
    v = scale * offset # RE OUTPUT: vec3 v_\d+ = \(scale\) \* \(offset\);
end
