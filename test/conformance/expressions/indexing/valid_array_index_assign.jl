function main()
    a = [1.5f0, 2.5f0, 3.5f0]
    i = 2

    a[1] = 0.5f0 # RE OUTPUT: a_\d+\[\(1\) - \(1\)\] = 0\.5
    a[i] = a[1]  # RE OUTPUT: a_\d+\[\(i_\d+\) - \(1\)\] = a_\d+\[\(1\) - \(1\)\];

    # COMMENT: implicitly converts to float in GLSL
    a[3] = 2
end
