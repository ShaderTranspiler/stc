# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): uniform capturing

# COMMENT: the generated array's length at transpilation time becomes the size of the captured variable's current size

# PRE EVAL BEGIN
A = [1.5f0, 2.5f0]
# PRE EVAL END

# OUTPUT: uniform float A[2];
function main()
    x = A[1] # OUTPUT: float x
end
