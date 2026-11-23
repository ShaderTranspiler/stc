# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: arrays can't be passed to or returned from methods

f(a::Vector{Float32}) = a[1] # ERROR: non-materializable

# ERROR(+1, missing): non-materializable
function g()::Vector{Float32}
    return [1.5f0, 2.5f0]
end
