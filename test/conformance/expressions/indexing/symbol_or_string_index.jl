# COMMENT: special error messages left over from pre-0.0.2 JuliaGLM swizzling

function main()
    v = Vec3F(0)

    x = v[:x]  # ERROR: Symbol
    y = v["x"] # ERROR: String
end
