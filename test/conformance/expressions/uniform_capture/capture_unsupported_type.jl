# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: only values of types that exist in GLSL can be captured as uniforms

# PRE EVAL BEGIN
S = "text"
# PRE EVAL END

function main()
    x = S # ERROR
end
