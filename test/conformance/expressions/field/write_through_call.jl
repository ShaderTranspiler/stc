# COMMENT: written by Claude, reviewed by szgerii

mutable struct Mutable
    x::Float32
end

create_temporary() = Mutable(1.5f0)

function main()
    # COMMENT: valid in Julia, but call result is not an l-value in GLSL
    create_temporary().x = 2.5f0 # ERROR(missing): l-value
end
