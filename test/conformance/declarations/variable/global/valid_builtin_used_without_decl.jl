# SHADER STAGE(frag)

# COMMENT: backend builtin globals make a special case, where their local assignment binds to the global symbol instead of creating a shadowing local (QoL feature)

function main()
    local x::Vec4F = gl_FragCoord # COMMENT: access
    gl_FragDepth = 0.5f0 # COMMENT: assign
end
