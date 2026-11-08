# COMMENT: written by Claude, reviewed by szgerii

# STC(missing)

# COMMENT: begin/end are treated as Int64 by Julia (and Int32 by us, known at compile time for every collection type)
# COMMENT: the same kind of flexibility should be allowed as on every other Int32 on an index
# COMMENT: we also currently don't do much compile-time bounds checking, since the GLSL compiler will reject it anyways

increment_index(x::Int32) = x + 1
decrement_index(x::Int32) = x - 1

function main()
    local a::Vector{Float32} = [1.5f0, 2.5f0, 3.5f0]

    res1 = a[end - 1]
    res2 = a[begin + 1]
    res3 = a[increment_index(begin)]
    res4 = a[decrement_index(end)]

    a[increment_index(begin)] = 0f0
    a[decrement_index(end)] = 0f0
end
