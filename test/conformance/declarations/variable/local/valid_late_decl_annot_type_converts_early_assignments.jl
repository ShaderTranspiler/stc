# STC(missing): redeclaration

function main()
    x = 0 # NOTE: implicitly converted to 0.0f0
    local x::Float32
    x = 1.5f0
end
