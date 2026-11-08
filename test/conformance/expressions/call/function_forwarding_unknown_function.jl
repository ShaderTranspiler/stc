# COMMENT: written by Claude, reviewed by szgerii

# STC FLAG(--fwd-fns)

# COMMENT: function that doesn't exist neither in Julia nor GLSL (with function forwarding enabled)

function main()
    local v::Vec3F = Vec3F(1)

    some_method_name_that_is_more_than_unlikely_to_ever_get_added_to_julia_std_lib(v) # ERROR: couldn't find function
end
