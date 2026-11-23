# COMMENT: written by Claude, reviewed by szgerii

global a::Float32 = 1.5f0
global b::Float32 = 2.5f0

function f()
    global a, b

    # COMMENT: a and b are the globals, so no new local may be declared for them
    
    a = 3.5f0 # RE OUTPUT: (?s)\A(?!.*float a_\d+ = 3\.5)
    b = 4.5f0 # RE OUTPUT(missing): (?s)\A(?!.*float b_\d+ = 4\.5)
end
