# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: broadcasting applies JuliaGLM's GLSL-style functions to each component separately (JuliaGLM.length.(v) is the absolute value of each component, JuliaGLM.dot.(v, w) is the component-wise product), which is not what the GLSL builtin computes on the whole vector in these cases
# COMMENT: how we want to handle the whole JuliaGLM vs Base resolution is not 100% clear at the moment, since both approaches have their advantages (for now, we try to match Julia semantics as closely as possible)

function main()
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    # ERROR(+1, missing): broadcast
    res1 = JuliaGLM.length.(v) # ERROR
    # ERROR(+1, missing): broadcast
    res2 = JuliaGLM.dot.(v, w) # ERROR
end
