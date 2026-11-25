# Stage one: the compressor map's three shape laws (and the choke scale) fitted to every cited map.
# Forms, stated before any number (25 Nov 14:05):
#   rise:  tau_knee = c(M) * 0.4 M^2,  c(M) = c0 + c1 (M - 1)            (a line in tip Mach)
#   knee:  phiK(M) = (k0 + k1 (max(M,0.8) - 1)) * kAr, clamped 0.15..0.9   (a line, the model's own form)
#   fall:  tau at choke = f * tau_knee, f constant (the model's ellipse from the knee to phi 1 kept)
#   choke: mc = k rho0 a0 A_ind arWide (0.3 + 0.7 min(M/1.63, 1))          (k one scale)
# The efficiency law is the model's, unchanged: eta = etaMax - 0.25 (M-1.1)^2 - 0.9 (phi - phiB)^2,
# phiB = phiK + 0.61 (1 - phiK); etaMax each map's stated peak.
# A drawn line's right end is where the efficiency falls to the contour P names (S400SX-E 0.54-0.64 -> 0.59; S300SX3
# 0.62-0.66 -> 0.64); a passenger map's "choke at the last contour" is the most flow at that ratio among the drawn lines
# with efficiency >= 0.58 (Garrett's stated definition; assumed for the EFR and the S400SX3).
# Residuals are divided by each reading's stated precision.
import math, sys, json
import numpy as np

A0 = math.sqrt(1.4 * 287.0 * 293.15)   # 343.2 m/s
RHO0 = 1.204
LBM = 0.00756

def area(d): return 0.91 * math.pi / 4 * d * d

HD = {
 'S400SX-E': dict(d=0.072, D=0.096, ar=0.6, etamax=0.80, eta_r=0.59, pw=0.7, pp=0.03, lines=[
   (230,1.41,12.0,45.4,1.22),(310,1.82,21.2,66.9,1.42),(370,2.25,28.6,81.2,1.63),(421,2.72,40.8,90.5,1.85),
   (461,3.21,47.1,96.6,2.05),(491,3.62,62.2,100.4,2.22),(521,4.10,64.8,103.2,2.40),(561,4.63,71.6,106.7,2.69),
   (607,4.99,78.5,110.3,3.08)]),
 'S300SX3': dict(d=0.06611, D=0.09144, ar=0.6, etamax=0.76, eta_r=0.64, pw=1.0, pp=0.03, lines=[
   (280,1.68,14.9,43.6,1.39),(370,2.34,25.7,60.7,1.78),(443,3.14,35.3,71.5,2.25),(505,3.94,46.2,75.7,2.84),
   (561,4.71,55.1,77.8,3.64)]),
}
PC = {  # per ratio: (PR, surge lb/min, choke at the last contour lb/min); top line tip speed m/s
 'GT2860RS': dict(d=0.047, ar=0.60, etamax=0.76, top=452, pw=0.5, rows=[(1.5,7.4,30.0),(2.0,10.9,33.1),(2.5,13.7,32.6)]),
 'GTX2860R': dict(d=0.046, ar=0.6, etamax=0.73, top=581, pw=0.5, rows=[(1.5,6.3,28.8),(2.0,9.8,38.4),(2.5,13.8,42.1),(3.0,16.4,39.6)]),
 'GT2560R': dict(d=0.046, ar=0.80, etamax=0.75, top=455, pw=0.5, rows=[(1.5,6.8,28.1),(2.0,10.4,34.3)]),
 'EFR6258': dict(d=0.049, ar=0.64, etamax=0.76, top=545, pw=0.5, rows=[(1.5,6.7,29.7),(2.0,10.0,38.7),(2.5,13.3,41.1),(3.0,19.0,41.5)]),
 'S400SX3': dict(d=0.07456, ar=0.6, etamax=0.78, top=551, pw=3.0, rows=[(1.5,22,86),(2.0,32.2,95.6),(2.5,39.5,99.5),(3.0,47.3,101.7)]),
}
ETA_LAST = 0.58

class Model:
    def __init__(s, p):
        s.c0, s.c1, s.k0, s.k1, s.f, s.k = p
    def c(s, M): return s.c0 + s.c1 * (M - 1.0)
    def tauK(s, M): return s.c(M) * 0.4 * M * M
    def phiK(s, M, ar): return min(max((s.k0 + s.k1 * (max(M, 0.8) - 1.0)) * (ar / 0.6) ** -0.3, 0.15), 0.9)
    def mc(s, M, d, ar): return s.k * RHO0 * A0 * area(d) * (ar / 0.6) ** 0.25 * (0.3 + 0.7 * min(M / 1.63, 1.0))
    def tau_at(s, phi, M, ar):
        tk = s.tauK(M); tc = s.f * tk; K = s.phiK(M, ar)
        y = (phi - K) / (1 - K)
        return tc + (tk - tc) * math.sqrt(max(1 - y * y, 1e-6))
    def eta(s, phi, M, ar, em):
        K = s.phiK(M, ar); pb = K + 0.61 * (1 - K)
        return em - 0.25 * (M - 1.1) ** 2 - 0.9 * (phi - pb) ** 2
    def right_end(s, M, ar, em, er):
        K = s.phiK(M, ar); pb = K + 0.61 * (1 - K)
        lo, hi = pb, 1.0
        if s.eta(1.0, M, ar, em) >= er: return 1.0
        for _ in range(40):
            mid = (lo + hi) / 2
            if s.eta(mid, M, ar, em) >= er: lo = mid
            else: hi = mid
        return (lo + hi) / 2

def pr(t): return (1 + max(t, -0.5)) ** 3.5
def tau_of(P): return P ** (1 / 3.5) - 1

def hd_residuals(m, name, out=None):
    g = HD[name]; r = []
    for U, pk, ws, wr, pr_r in g['lines']:
        M = U / A0
        mc = m.mc(M, g['d'], g['ar']) / LBM
        K = m.phiK(M, g['ar'])
        p_pk = pr(m.tauK(M)); w_s = K * mc
        phr = m.right_end(M, g['ar'], g['etamax'], g['eta_r'])
        w_r = phr * mc; p_r = pr(m.tau_at(phr, M, g['ar']))
        rr = [(p_pk - pk) / g['pp'], (w_s - ws) / g['pw'], (w_r - wr) / g['pw'], (p_r - pr_r) / g['pp']]
        r += rr
        if out is not None: out.append((name, U, pk, p_pk, ws, w_s, wr, w_r, pr_r, p_r))
    return r

def pc_residuals(m, name, out=None):
    g = PC[name]; r = []
    Mtop = g['top'] / A0
    for P, ws, wc in g['rows']:
        t = tau_of(P)
        # the surge line: the knee whose rise is this ratio
        lo, hi = 0.2, 3.0
        for _ in range(50):
            mid = (lo + hi) / 2
            if m.tauK(mid) < t: lo = mid
            else: hi = mid
        Ms = (lo + hi) / 2
        w_s = m.phiK(Ms, g['ar']) * m.mc(Ms, g['d'], g['ar']) / LBM
        # the choke at the last contour: the most flow at this ratio among drawn lines, efficiency >= 0.58
        best = 0.0
        for i in range(120):
            M = Ms + (Mtop - Ms) * i / 119.0
            if M > Mtop or m.tauK(M) < t: continue
            K = m.phiK(M, g['ar'])
            if m.tau_at(1.0, M, g['ar']) > t: continue  # the line ends above this ratio
            a, b = K, 1.0
            for _ in range(40):
                mid = (a + b) / 2
                if m.tau_at(mid, M, g['ar']) > t: a = mid
                else: b = mid
            phi = (a + b) / 2
            if m.eta(phi, M, g['ar'], g['etamax']) >= ETA_LAST:
                best = max(best, phi * m.mc(M, g['d'], g['ar']) / LBM)
        r += [(w_s - ws) / g['pw'], (best - wc) / g['pw']]
        if out is not None: out.append((name, P, ws, w_s, wc, best))
    return r

def resid(p, hd, pc):
    m = Model(p); r = []
    for n in hd: r += hd_residuals(m, n)
    for n in pc: r += pc_residuals(m, n)
    return np.array(r)

def nm(fun, x0, steps, iters=4000):
    n = len(x0); pts = [np.array(x0, float)]
    for i in range(n):
        x = np.array(x0, float); x[i] += steps[i]; pts.append(x)
    vals = [fun(x) for x in pts]
    for it in range(iters):
        o = np.argsort(vals); pts = [pts[i] for i in o]; vals = [vals[i] for i in o]
        if abs(vals[-1] - vals[0]) < 1e-9 * (1 + abs(vals[0])): break
        cen = np.mean(pts[:-1], axis=0)
        xr = cen + (cen - pts[-1]); fr = fun(xr)
        if fr < vals[0]:
            xe = cen + 2 * (cen - pts[-1]); fe = fun(xe)
            if fe < fr: pts[-1], vals[-1] = xe, fe
            else: pts[-1], vals[-1] = xr, fr
        elif fr < vals[-2]: pts[-1], vals[-1] = xr, fr
        else:
            xc = cen + 0.5 * (pts[-1] - cen); fc = fun(xc)
            if fc < vals[-1]: pts[-1], vals[-1] = xc, fc
            else:
                for i in range(1, len(pts)):
                    pts[i] = pts[0] + 0.5 * (pts[i] - pts[0]); vals[i] = fun(pts[i])
    i = int(np.argmin(vals)); return pts[i], vals[i]

def fit(hd, pc, x0=(0.5, -0.1, 0.40, 0.178, 0.35, 0.45)):
    f = lambda p: float(np.sum(resid(p, hd, pc) ** 2))
    best = None
    for start in (x0, (0.55, -0.1, 0.35, 0.4, 0.6, 0.48), (0.5, 0.0, 0.3, 0.6, 0.5, 0.45)):
        x, v = nm(f, start, [0.05, 0.05, 0.05, 0.1, 0.1, 0.05])
        x, v = nm(f, x, [0.02, 0.02, 0.02, 0.05, 0.05, 0.02])
        if best is None or v < best[1]: best = (x, v)
    return best

def rms(p, hd, pc):
    r = resid(p, hd, pc); return math.sqrt(np.mean(r ** 2)), len(r)

if __name__ == '__main__':
    today = (0.45, 0.0, 0.40, 0.178, 0.35, 0.45)
    res = {}
    allhd, allpc = list(HD), list(PC)
    for tag, hd, pc in (('all maps', allhd, allpc), ('heavy-duty (P\'s two)', allhd, []),
                        ('passenger + S400SX3 (the knee law\'s maps)', [], allpc),
                        ('S400SX-E alone', ['S400SX-E'], []), ('S300SX3 alone', ['S300SX3'], [])):
        x, v = fit(hd, pc)
        res[tag] = x
        print(f"{tag}: c0 {x[0]:.3f} c1 {x[1]:+.3f} k0 {x[2]:.3f} k1 {x[3]:+.3f} f {x[4]:.3f} k {x[5]:.3f}; rms {rms(x, hd, pc)[0]:.2f} over {rms(x,hd,pc)[1]} residuals (in reading precisions)")
    print(f"today's model on all: rms {rms(today, allhd, allpc)[0]:.2f}; on P's two: {rms(today, allhd, [])[0]:.2f}; on the knee law's maps: {rms(today, [], allpc)[0]:.2f}")
    print("hold-out: S400SX-E's fit on the S300SX3: rms %.2f; S300SX3's fit on the S400SX-E: rms %.2f" % (rms(res['S400SX-E alone'], ['S300SX3'], [])[0], rms(res['S300SX3 alone'], ['S400SX-E'], [])[0]))
    print("cross-class: heavy-duty fit on the knee law's maps: rms %.2f; the knee law's maps' fit on P's two: rms %.2f" % (rms(res["heavy-duty (P's two)"], [], allpc)[0], rms(res["passenger + S400SX3 (the knee law's maps)"], allhd, [])[0]))
    json.dump({k: list(map(float, v)) for k, v in res.items()}, open(sys.argv[1] if len(sys.argv) > 1 else '/dev/null', 'w'))
    for tag in ('all maps', "heavy-duty (P's two)"):
        m = Model(res[tag]); print("\nresiduals, fit: " + tag)
        out = []
        for n in allhd: hd_residuals(m, n, out)
        print("map       U    peakPR pub/mod   surge lb/min pub/mod   right lb/min pub/mod   right PR pub/mod")
        for o in out: print("%-9s %3d  %.2f/%.2f      %5.1f/%5.1f          %5.1f/%5.1f         %.2f/%.2f" % o)
        out = []
        for n in allpc: pc_residuals(m, n, out)
        print("map        PR   surge pub/mod   choke@last pub/mod")
        for o in out: print("%-9s %.1f  %5.1f/%5.1f    %5.1f/%5.1f" % o)
