# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid Julia, declares a function without any methods

# ERROR(+1, missing)
function f end # INTERNAL ERROR: unexpected function definition layout
