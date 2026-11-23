# ERROR(+1, missing): non-materializable
function f()::String # ERROR: type mismatch
end

# ERROR(+1, missing): non-materializable
function g()::Symbol # ERROR: type mismatch
end
