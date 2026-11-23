# COMMENT: written by Claude, reviewed by szgerii

@gl_const global SCALE::Float32 = 2.5f0 # OUTPUT: const float SCALE
@gl_const global EPS = 0.001f0          # OUTPUT: const float EPS

function main()
    x = SCALE * EPS
end
