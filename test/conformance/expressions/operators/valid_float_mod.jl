function main()
    local x::Float32 = 1.0f0
    local y::Float32 = 2.0f0

    # RE OUTPUT: float res1_\d+ = mod\(x_\d+, y_\d+\) 
    res1 = mod(x, y)
end
