# RE OUTPUT: float f_\d+\(\)
f()::Float32 = 1.0f0
# RE OUTPUT: int g_\d+\(int x_\d+\)
g(x::Int32)::Int32 = x
# RE OUTPUT: float h_\d+\(\)
h()::Float32 = 1 # COMMENT: without the return type annotation, this would get inferred as returning Int
