# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: this is rejected, because most cases would throw anyways (scalars are not broadcastable, vectors and matrices are immutable)
# COMMENT: maybe in the future, we could support it for arrays?

function main()
    local v::Vec3F = Vec3F(1)
    local w::Vec3F = Vec3F(2)

    v .= w # ERROR: broadcasting on regular assignment
end
