function f()::Float32
    # COMMENT: implicit
    return 1    # OUTPUT: return 1;

    # COMMENT: explicit
    return true # OUTPUT: return float(true);
end
