# COMMENT: written by Claude, reviewed by szgerii

mutable struct P
    x::Float32
end

function main()
    p = P(0f0)
    q = P(0f0)
    arr = [0f0, 0f0]

    p.x = q.x = 1.5f0
    arr[1] = arr[2] = 2.5f0
end
