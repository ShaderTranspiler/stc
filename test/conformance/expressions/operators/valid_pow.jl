# STC(missing)

# COMMENT: because of pow() in GLSL defining its domain differently than Julia's ^ operator, we separate literal and runtime cases, and try to unfold to multiplication when possible

function main()
    local x::Int32 = 2
    x^0 # RE OUTPUT(multiline): ^\s*1;
    x^2 # RE OUTPUT: \(x_\d+\) \* \(x_\d+\)
end
