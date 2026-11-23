# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): undeclared

# COMMENT: the condition is evaluated in the enclosing scope, so the body's 'local x' doesn't apply to it, thus it should be able to resolve this
# COMMENT: i.e. x in condition should bind to the enclosing scope's x and not treat is as the local variable used before declaration

function main()
    x = 0

    while x < 3
        local x = 5
    end

    y = x
end
