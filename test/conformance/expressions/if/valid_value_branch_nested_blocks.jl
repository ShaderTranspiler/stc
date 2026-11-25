# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: blocks nested in a value branch still hold a single expression, so they unwrap all the way down

function main()
    local c::Bool = true

    # RE OUTPUT: int x_\d+ = \(c_\d+\) \? \(1\) : \(2\);
    x = if c
        begin
            1
        end
    else
        begin
            begin
                2
            end
        end
    end

    # RE OUTPUT: int y_\d+ = \(c_\d+\) \? \(3\) : \(4\);
    y = c ? begin begin begin 3 end end end : 4
end
