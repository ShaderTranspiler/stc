function main()
    local x::Float32 = 0.0f0
    x = Int32(1) # COMMENT: Julia keeps the Float32 type of the variable, and GLSL supports int -> float implicit conversion
end
