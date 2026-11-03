struct S
    x::Float32
    y::Float32
end

function main()
    s = S(1.5f0, 2.5f0) # RE OUTPUT: S s_\d+ = S\(1\.5\d*f, 2\.5\d*f\);
end
