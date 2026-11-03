# COMMENT: this should be lowerable to values.length()
# STC(missing): length

@gl_layout std430 binding=0 @gl_buffer struct Data
    values::Vector{Float32}
end

function main()
    n = length(values)
end
