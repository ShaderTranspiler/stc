# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: mangled names never collide with GLSL keywords, so they're allowed

global filter::Float32 = 1.5f0

input() = 1.5f0

function main()
    sample = 1.5f0
    output = input()
    vec2 = filter
end
