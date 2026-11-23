# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): undeclared global binding

# PRE EVAL BEGIN
V = Vec3F(1.5f0, 2.5f0, 3.5f0)
# PRE EVAL END

# OUTPUT: uniform vec3 V;

function main()
    x = V # OUTPUT: vec3 x
end
