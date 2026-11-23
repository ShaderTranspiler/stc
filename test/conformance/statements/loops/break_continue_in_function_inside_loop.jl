function main()
    while true
        function f()
            break    # ERROR: outside of loop
            continue # ERROR: outside of loop
        end
    end
end
