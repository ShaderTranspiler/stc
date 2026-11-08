# COMMENT: written by Claude, reviewed by szgerii

function main()
    local x::Float32 = 1.5f0
    Foo.bar(x) # ERROR: couldn't resolve module
end
