@gl_buffer struct Data
    x::Float32
end

function main()
    d = Data(1.5f0) # ERROR: couldn't find function 'Data'
end
