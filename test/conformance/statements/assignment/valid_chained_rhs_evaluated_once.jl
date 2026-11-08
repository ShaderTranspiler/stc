# COMMENT: written by Claude, reviewed by szgerii

f(x::Float32) = x

function main()
    local a::Float32 = 0f0
    local b::Float32 = 0f0

    # COMMENT: make sure a call to f is only produced once (argument is included so that it's distinguishable from the declaration)
    # RE OUTPUT: = f_\d+\(1\.5\d*f\);
    # RE OUTPUT: (?s)\A(?!(.*?f_\d+\(1\.5\d*f\);){2})
    a = b = f(1.5f0)
end
