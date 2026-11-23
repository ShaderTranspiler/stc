#=
    this is a known deviation from Julia (which would throw on line 9)
    in GLSL, reading a local after declaration, but before assignment is allowed, but the value is undefined
    real detection would need the IR, so for now it's marked here as a valid case
=#

function main()
    local x::Float32
    y = x + 1
    x = 0.5f0
end
