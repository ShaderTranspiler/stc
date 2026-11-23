# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): undeclared global binding

# COMMENT: every use refers to the same captured uniform

# PRE EVAL BEGIN
K = 1.5f0
# PRE EVAL END

# COMMENT: check that there is only a single uniform produced
# OUTPUT: uniform float K;
# RE OUTPUT: (?s)\A(?!(?:.*?uniform float K;){2})

function f()::Float32
    return K
end

function main()
    x = K + f()
end
