# COMMENT: written by Claude, reviewed by szgerii

g = 1.5f0
global g::Float32 = 2.5f0 # ERROR(missing): redeclaration

function main()
    x = g
end
