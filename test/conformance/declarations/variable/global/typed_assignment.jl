# COMMENT: written by Claude, reviewed by szgerii

# COMMENT: valid in Julia (declares a typed global), but stc requires the explicit global form
# COMMENT: this is the general problem with distinguishing type assertions from short declarations, which is its own issue we completely reject for now

x::Float32 = 1.5f0 # ERROR: declarations of the form
