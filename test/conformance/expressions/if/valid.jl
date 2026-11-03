# COMMENT: just meant to test basic if/elseif/else syntax with compounds, uses calls because they have no effect on the local binding table of 'main' / if scoping

f() = nothing
f(x::Int32) = nothing
f(x::Float32, y::Float32) = nothing

function main()
    local some_bool::Bool = true
    
    if some_bool
        f()
    end

    if some_bool
        f()
        f(1)
        f(1.5f0, 0.5f0)
    else
        f(1)
    end

    if some_bool
        f(1.5f0, 0.5f0)
        f(1.0f0, 2.25f0)
    elseif false
        f()
        f(2)
    end

    if false
        f()
        f(0.5f0, 1.5f0)
        f()
    elseif some_bool
        f(1)
        f()
        f(4)
    elseif true
        f()
        f(1.5f0, 0.5f0)
    else
        f()
    end
end
