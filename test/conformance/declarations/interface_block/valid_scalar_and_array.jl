@gl_layout std430 binding=0 @gl_buffer struct Data
    scale::Float32
    values::Vector{Float32}
end

function main()
    idx = gl_GlobalInvocationID.x + UInt32(1)
    values[idx] = values[idx] * scale
end
