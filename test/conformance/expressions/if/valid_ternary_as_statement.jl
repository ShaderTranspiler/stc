f() = 1.0f0
g() = 2.0f0

function main()
    some_bool = true

    # RE OUTPUT: f_\d+\(\);
    # RE OUTPUT: g_\d+\(\);
    some_bool ? f() : g()

    # RE OUTPUT: \{\s*\}
    some_bool ? f() : nothing
    some_bool ? nothing : f()
    some_bool ? nothing : nothing
end
