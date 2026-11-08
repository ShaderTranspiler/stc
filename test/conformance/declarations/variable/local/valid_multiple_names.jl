# COMMENT: written by Claude, reviewed by szgerii

function main()
    # RE OUTPUT: float a_\d+;
    # RE OUTPUT(missing): float b_\d+;
    local a::Float32, b::Float32

    a = 1.5f0 # RE OUTPUT(multiline): ^(\s*)a_\d+ = 1\.5\d*f;
    b = 2     # RE OUTPUT(multiline, missing): ^(\s*)b_\d+ = 2;
end
