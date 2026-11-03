function main()
    local x::Int32

    if true
        x = 2
    end

    y = x # RE OUTPUT: int y_\d+ = x_\d+;
end
