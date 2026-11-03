# COMMENT: void return type

function f()
    nothing
end

function g()::Nothing
    return nothing
end

function h()::Int32
    nothing # COMMENT: nothing is a valid statement in itself (stripped from generated code)
    return 0
end
