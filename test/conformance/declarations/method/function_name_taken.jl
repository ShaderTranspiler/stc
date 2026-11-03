global f::Int32 = 0

f(x::Int32) = nothing # ERROR: 'f'

gl_FragCoord(x::Int32) = nothing # ERROR(missing): 'gl_FragCoord'

