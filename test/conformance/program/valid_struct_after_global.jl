struct Struct1
    x::Int32
end

# COMMENT: transpiler should lift all struct decls to the top
some_global = 0.0f0

struct Struct2
    x::Int32
end

function main()
end

