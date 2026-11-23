# COMMENT: written by Claude, reviewed by szgerii

# STC(missing): forward capture

# COMMENT: z is also assigned after the loop, which makes it a local of main, so the assignment in the body should update that variable

function main()
    i = 0

    while i < 3
        i += 1
        z = i
    end

    z = 1
end
