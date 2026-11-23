# STC(missing): unsupported Julia type: S

# COMMENT: Julia only resolves the return type annotation at "return time", so this is allowed, as long as S is defined before the first call to g

function g()::S
    return S(1.5f0)
end

struct S
    x::Float32
end

function main()
    g()
end
