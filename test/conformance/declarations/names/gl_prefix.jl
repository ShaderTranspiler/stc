# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: GLSL reserves the gl_ prefix, current mangling doesn't help since the prefix stays

global gl_g::Float32 = 1.5f0     # ERROR(missing): gl_
@gl_uniform global gl_u::Float32 # ERROR(missing): gl_

struct gl_S # ERROR(missing): gl_
    x::Float32
end

struct S
    gl_x::Float32 # ERROR(missing): gl_
end

gl_f() = 1.5f0          # ERROR(missing): gl_
f(gl_p::Float32) = gl_p # ERROR(missing): gl_

function main()
    gl_x = 1.5f0                # ERROR(missing): gl_
    local gl_y::Float32 = 1.5f0 # ERROR(missing): gl_
end
