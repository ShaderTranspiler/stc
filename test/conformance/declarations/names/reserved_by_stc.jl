# COMMENT: written by Claude, reviewed by szgerii

struct gl # ERROR: invalid identifier
    x::Float32
end

struct S
    gl::Float32 # ERROR: invalid identifier
end

gl() = 1.5f0        # ERROR: invalid function identifier
f(gl::Float32) = gl # ERROR: invalid identifier

function main()
    gl = 1.5f0        # ERROR: invalid identifier
    stc_tmp_0 = 1.5f0 # ERROR: invalid identifier
end
