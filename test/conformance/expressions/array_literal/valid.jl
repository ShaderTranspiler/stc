# COMMENT: written by Claude, reviewed by szgerii

function main()
    a1 = [1.5f0, 2.5f0, 3.5f0] # RE OUTPUT: float a1_\d+\[3\] = float\[3\]\(
    a2 = [true, false]         # RE OUTPUT: bool a2_\d+\[2\] = bool\[2\]\(true, false\);
    a3 = [1, 2, 3]             # RE OUTPUT: int a3_\d+\[3\] = int\[3\]\(1, 2, 3\);
    a4 = [0.5f0]               # RE OUTPUT: float a4_\d+\[1\] = float\[1\]\(
end
