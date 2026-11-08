# COMMENT: written by Claude, reviewed by szgerii

function main()
    if true
        local x = 2
    end

    y = x # ERROR(missing): local
end
