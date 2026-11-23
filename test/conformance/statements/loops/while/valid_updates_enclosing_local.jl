# COMMENT: written by Claude, reviewed by szgerii

function main()
    x = 0

    while x < 3
        # COMMENT: the body updates the x of main, it doesn't declare a new local
        x += 1 # RE OUTPUT(multiline): ^(\s*)x_\d+ \+= 1;
    end

    y = x
end
