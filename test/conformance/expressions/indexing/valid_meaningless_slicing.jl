# COMMENT: these all produce the exact same value as their target array

# STC(missing)

function main()
    a = [1, 2, 3, 4]

    b = a[begin:end] # RE OUTPUT: int b_\d+\[4\] =
    c = a[:]         # RE OUTPUT: int c_\d+\[4\] =

    # COMMENT: this is treated as a simple element access in Julia (but only with literals)
    d = a[:3]        # OUTPUT: int d
end
