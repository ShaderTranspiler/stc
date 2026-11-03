@gl_buffer struct Data
    x::Float32
end

function main()
    local d::Data # ERROR: unsupported Julia type
end
