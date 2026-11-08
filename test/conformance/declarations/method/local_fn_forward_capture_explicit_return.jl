# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: same as the inferred return type case, but methods with an explicit return type are typed later, which currently hides the forward capture behind the general capture error

function f()::Float32
    # ERROR(+2, missing): forward capture
    # ERROR(+1): captured
    function g()::Float32
        return x
    end

    x = 1.5f0
    return g()
end
