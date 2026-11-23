function f(some_bool::Bool)
    if some_bool
        return 1.5f0
    end

    return 2 # ERROR(missing): type mismatch
end

function g(some_bool::Bool)
    if some_bool
        return 2
    end

    return 1.5f0 # ERROR: type mismatch
end
