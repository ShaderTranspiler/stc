# STC(missing): cannot assign bool to f32

function main()
    a = [1.5f0, 2.5f0]

    # COMMENT: Julia converts here, and bool -> float is a valid explicit conversion in GLSL, so this should be possible
    a[1] = true
end
