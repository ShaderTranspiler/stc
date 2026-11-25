# COMMENT: same case as return_value_branch_types_differ.jl, but with an annotated return type, which makes the return type stable

function h(c::Bool)::Float32
    return c ? 1 : 1.5f0
end
