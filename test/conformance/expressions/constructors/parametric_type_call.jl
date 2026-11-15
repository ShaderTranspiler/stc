# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: parametric types are unsupported for now as a whole, so calling a constructor through one should be rejected (otherwise, this is valid Julia: JuliaGLM's Vec3 is Vec3T{Float32})

function main()
    v = Vec3T{Float32}(1f0, 2f0, 3f0) # ERROR: unsupported Expr node
end
