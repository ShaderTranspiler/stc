struct S
    x::Float32
    i::Int32
end

global s1::S = S(0.5f0, 1) # RE OUTPUT: S s1_\d+ = (.|\n)+void main

s2 = S(1.5f0, 2) # RE OUTPUT: S s2_\d+ = (.|\n)+void main

function main()
end
