# COMMENT: written by Claude, reviewed by szgerii

# SHADER STAGE(frag)

@gl_in  global uv::Vec2F  # OUTPUT: in vec2 uv;
@gl_out global col::Vec4F # OUTPUT: out vec4 col;

function main()
    global col
    col = Vec4F(uv.x, uv.y, 0.0f0, 1.0f0)
end
