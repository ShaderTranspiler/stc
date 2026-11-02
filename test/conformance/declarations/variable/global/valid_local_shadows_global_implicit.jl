global x::Float32 = 0.0f0

# COMMENT: this is the case where from a method the binding for 'x' is inferred through assignment, in which case Julia will shadow the global with a new local

# TODO: uncomment OUTPUT after directive has been implemented
# COMMENT: OUTPUT: x_0
# COMMENT: OUTPUT: x_1
function main()
    x = 0.0f0
end
