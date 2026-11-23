# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): undeclared global binding

#=
    K is not bound in source code, but accessible through the Julia runtime as a global, so it's captured as a uniform (without a value)

    In the future, it would be useful if the transpiler also produced a list of captured uniforms, along with type info so that callers can actually do stuff dynamically with this information,
    without having to do their own inference.
    But we also wanna move towards constant baking as the default option, since it seems more useful in most scenarios, so this is low-prio.
=#

# PRE EVAL BEGIN
K = 1.5f0
# PRE EVAL END

# OUTPUT: uniform float K;

function main()
    x = K # OUTPUT: float x
end
