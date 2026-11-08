# COMMENT: written by Claude, reviewed by szgerii

f(xs::Int32...) = 1 # ERROR: variadic
g(xs...) = 1        # ERROR: variadic
