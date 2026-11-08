# COMMENT: written by Claude, reviewed by szgerii

function main()
    # RE OUTPUT: int x_\d+ = 1;
    # RE OUTPUT(multiline): ^(\s*)int y_\d+ = \(x_\d+\) \+ \(1\);
    x = 1; y = x + 1
end
