# COMMENT: written by Claude, reviewed by szgerii

x! = 1.5f0 # ERROR(missing)

set!(p!::Float32) = p! # ERROR(missing)

function main()
    y! = 1.5f0 # ERROR(missing)
end
