# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: outside of interactive contexts, an assignment in a top-level while body declares a new local instead of updating the global (Julia warns about the ambiguity, then throws an UndefVarError at runtime)

x = 0

while x < 3
    x += 1 # ERROR: undeclared
end
