# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: extending methods of other modules is not supported

# ERROR(+1, missing)
Base.sin(x::Int32) = x # INTERNAL ERROR: non-symbol name
