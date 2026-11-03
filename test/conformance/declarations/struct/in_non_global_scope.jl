function main()
    struct S # ERROR: non-global scope
        x::Float32
    end
end
