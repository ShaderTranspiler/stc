# ERROR(+1): type mismatch
function f()::Float32
end

function g(some_condition::Bool)::Float32
    if some_condition # ERROR: type mismatch
        return 1.5f0
    end
end

function h(some_condition::Bool)::Bool
    if some_condition
        return false
    end

    x = 1.5f0 # ERROR: type mismatch
end
