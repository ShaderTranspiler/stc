using Libdl

# ! REMINDER
# in the real pkg, the transpile ccall will have to be wrapped in GC.@preserve

const LIB_PATH = joinpath(@__DIR__, "..", "build", "vscode", "lib", "libstc_lib.dll")

cpp_free(ptr) = ccall((:stc_jl_free, LIB_PATH), Cvoid, (Ptr{Cvoid},), ptr)

function test_call(input_expr)
    c_str = ccall((:stc_jl_print_expr, LIB_PATH), Ptr{Cchar}, (Any,), input_expr)

    if c_str == C_NULL
        @warn "An error occured during the cpp lib call"
        return "julia default return string"
    end

    str = unsafe_string(c_str)

    ccall((:stc_jl_free, LIB_PATH), Cvoid, (Ptr{Cchar},), c_str)

    return str
end

function cpp_dump(expr::Expr)
    GC.@preserve expr begin
        ccall((:stc_jl_parse_expr, LIB_PATH), Cvoid, (Any,), expr)
    end
end

# test_call(:(
#     if x == 0
#         println("x is zero")
#     elseif x < 0
#         println("x is less than zero")
#     else
#         println("x is greater than zero")
#     end
# ))

@time cpp_dump(quote
    a = 4

    global x::Int = 2
    y = false

    function f(x::Int)
        function g()
            z = 3
        end


        z = 3

        g()

        return 2

    end

    if true
        f(1)
    elseif y
        f(2)
    else
        f(3)
    end
end)
