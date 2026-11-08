# COMMENT: written by Claude, reviewed by szgerii

function main()
    f = x -> x + 1 # ERROR: arrow functions

    g = function (x) # ERROR: anonymous functions
        x
    end
end
