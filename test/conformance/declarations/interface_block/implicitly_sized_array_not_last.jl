@gl_buffer struct Data
    arr::Vector{Float32} # ERROR: implicitly sized array
    x::Float32
end
