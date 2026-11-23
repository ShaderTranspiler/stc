struct Struct1
    x::Int32
end

function f()
end

# COMMENT: transpiler should lift all struct decls to the top
struct Struct2
    x::Int32
end
