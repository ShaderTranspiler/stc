function main()
    @gl_buffer struct Data # ERROR: non-global scope
        x::Float32
    end
end
