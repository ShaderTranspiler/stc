# COMMENT: written by Claude, reviewed by szgerii

# GLSL(missing): wrong operand types

# COMMENT: GLSL's & and | don't take bools, so & and | on Bool are lowered to the logical operators

function main()
    local a::Bool = true
    local b::Bool = false

    res1 = a & b     # RE OUTPUT(missing): bool res1_\d+ = \(a_\d+\) && \(b_\d+\);
    res2 = a | b     # RE OUTPUT(missing): bool res2_\d+ = \(a_\d+\) \|\| \(b_\d+\);
    res3 = xor(a, b) # RE OUTPUT: bool res3_\d+ = \(a_\d+\) \^\^ \(b_\d+\);
end
