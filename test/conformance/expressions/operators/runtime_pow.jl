# COMMENT: GLSL only has float overloads for pow, so we'd have to do int(pow(float(x), float(y))) but that probably drifts with scale

function main()
    local x::Int32 = 2
    local n::Int32 = 3
    x^n # ERROR: ^
end
