# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: the return type isn't annotated, so it's inferred from the explicitly returned if, whose branches disagree

function h(c::Bool)
    return c ? 1 : 1.5f0 # ERROR
end
