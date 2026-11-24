# knock_tables.py: writes the three tables of knock.fm (python3 knock_tables.py > tables.txt, then paste them over
# knock.fm's). Each is sampled evenly for a linear read: the Arrhenius factor of Douaud and Eyzat's delay, exp(-3800 /
# T) for T = 250 + 5 i K; the pressure's power, p^1.7 for p = i / 2 atm; the end gas's isentropic temperature ratio,
# r^((g - 1) / g) = r^0.25 (g = 4/3) for the pressure ratio r = i / 4.
import math
def num(v):
    t = "%.7g" % v
    if "." not in t and "e" not in t:
        t += ".0"
    return t + "f"

def fmt(vals, per=8):
    s=[]
    for i in range(0,len(vals),per):
        s.append("    " + ", ".join(num(v) for v in vals[i:i+per]) + ",")
    s[-1]=s[-1].rstrip(",")
    return "\n".join(s)
# exp(-3800/T), T = 250 + 5 i, 257 entries
de=[math.exp(-3800.0/(250+5*i)) for i in range(257)]
# p^1.7, p = 0.5 i atm, 513 entries
p17=[(0.5*i)**1.7 for i in range(513)]
# r^0.25, r = 0.25 i, 513 entries
ru=[(0.25*i)**0.25 for i in range(513)]
print(
 "const float kKnockArr[257] = {\n"+fmt(de)+"};\n\nconst float kKnockPow[513] = {\n"+fmt(p17)+"};\n\nconst float kKnockGas[513] = {\n"+fmt(ru)+"};\n")
