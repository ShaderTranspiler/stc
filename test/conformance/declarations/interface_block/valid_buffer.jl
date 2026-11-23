# COMMENT: written by Claude, reviewed by szgerii

# OUTPUT: layout(std430, binding = 0) buffer
@gl_layout std430 binding=0 @gl_buffer struct Data
    values::Vector{Float32}
end

function main()
    # COMMENT: invocation ids are zero-based, but Julia's indexing still starts from one
    idx = gl_GlobalInvocationID.x + UInt32(1)

    # RE OUTPUT: values\[\(idx_\d+\) - \(1u\)\] = \(values\[\(idx_\d+\) - \(1u\)\]\) \* \(2\.0+f\);
    values[idx] = values[idx] * 2.0f0
end
