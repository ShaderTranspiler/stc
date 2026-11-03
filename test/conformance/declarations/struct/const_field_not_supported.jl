mutable struct S
    # ERROR(+1, missing): constant fields are currently not supported
    const x::Float32 # ERROR: inner constructors
end
