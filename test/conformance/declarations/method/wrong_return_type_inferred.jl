function f(some_condition::Bool)
    if some_condition
        return true
    end

    return 1.5f0 # ERROR: type mismatch
end
