# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: as statements, ternaries lower to if statements, so control flow inside their branches is fine
# COMMENT: this can seem a bit arbitrary at first (given that the 'x = if i > 3 break else nothing end' would error), but given that
# COMMENT: 'a ? begin b end : begin c end' gives the exact same AST 'if a b else c end' gives, one can see there's virtually no difference between ifs and ternaries in Julia
# COMMENT: we choose to handle ternaries by requiring a) both branches are given, b) one expression per branch, c/1) both branches infer the same type, or c/2) both branches can be checked to have the same type
# COMMENT: this means we cut out a specific subset of 'if'-s to treat as ternaries, which naturally will allow some weird cases like the one below 

function f(c::Bool)::Int32
    c ? (return Int32(1)) : nothing
    return Int32(2)
end

function main()
    local i::Int32 = 0

    while true
        i += 1
        i > 3 ? (break) : nothing
        i == 2 ? (continue) : nothing
    end
end
