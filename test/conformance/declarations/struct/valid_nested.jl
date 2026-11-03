struct Inner
    x::Float32
end

struct Outer
    inner::Inner
    y::Float32
end

function main()
    o = Outer(Inner(1.5f0), 2.5f0) # RE OUTPUT: Outer o_\d+ = Outer\(
    i = o.inner # RE OUTPUT: Inner i_\d+ = \(o_\d+\)\.inner;
end
