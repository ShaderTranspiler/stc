# GLSL(missing): unexpected IF

function main()
    some_bool = true
    x = some_bool ? 1.0 : 2.0
    x = some_bool ? begin 1.0 end : begin 2.0 end
end
