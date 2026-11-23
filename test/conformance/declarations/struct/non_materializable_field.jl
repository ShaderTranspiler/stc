# COMMENT: written by Claude, reviewed by szgerii

struct S1
    x::Nothing # ERROR: non-materializable
end

struct S2
    x::Vector{Float32} # ERROR: non-materializable
end

struct S3
    x::String # ERROR: non-materializable
end

struct S4
    x::Symbol # ERROR: non-materializable
end
