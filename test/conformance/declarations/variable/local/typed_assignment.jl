# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid in Julia (declares a typed local), but stc requires the explicit local form for now

function main()
    x::Float32 = 1.5f0 # ERROR: declarations of the form
end
