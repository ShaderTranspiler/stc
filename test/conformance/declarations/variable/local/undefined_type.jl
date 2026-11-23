# COMMENT: written by Claude, reviewed by szgerii

function main()
    # ERROR(+1, missing): undefined
    local x::Foo = 1 # ERROR: unsupported Julia type: Foo
end
