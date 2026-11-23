# COMMENT: written by Claude, reviewed by szgerii

# STC FLAG(--coerce-non-finite)

# COMMENT: with the flag, Float64 literals that overflow Float32 under coercion become inf instead of an error

function main()
    # COMMENT: no constants in GLSL, so we generate them from the bits as uint
    a = 1e40  # RE OUTPUT: float a_\d+ = uintBitsToFloat\(0x7F800000u\);
    b = -1e40 # RE OUTPUT: float b_\d+ = uintBitsToFloat\(0xFF800000u\);
end
