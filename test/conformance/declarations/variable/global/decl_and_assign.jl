global x::Int32 = 0

function main()
    global x = 1 # ERROR
end
