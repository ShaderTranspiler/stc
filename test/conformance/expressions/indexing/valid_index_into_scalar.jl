# STC(missing)
# COMMENT: Julia allows this (numbers are iterable, x[1] == x if x is a scalar), the transpiler should allow it too and lower it to the target expression simply

function main()
    x = 1.5f0
    y = x[1] # RE OUTPUT: float y_\d+ = x_\d+;
end
