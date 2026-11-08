# COMMENT: written by Claude, reviewed by szgerii

# STC FLAG(--fwd-fns, -Wjl-query)
# GLSL(missing, intended): no matching overloaded function

# COMMENT: with forwarding, Julia infers the return type and the call is passed on as it is, even though GLSL has no hypot (forwarding may produce invalid code by design)

function main()
    local x::Float32 = 1.5f0

    # WARNING(+2): function reference through julia
    # WARNING(+1): return type through julia
    res = hypot(x, x) # RE OUTPUT: float res_\d+ = hypot\(x_\d+, x_\d+\);
end
