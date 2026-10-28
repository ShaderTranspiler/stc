# COMMENT: fns with return type annotation currently require longdef signature
function f(x::Float32)::Float32
    f(x) # ERROR(missing): recursion
end