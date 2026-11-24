# The cylinder gas's ratio of specific heats: where 1.392 - 8.13e-5 T + 0.037 (1 - φ) comes from

*Card N3, 2026-11-24 (20:00 to 20:10 by `date -u`). For `Cylinder::gammaAt` (card D's `sim/core/cylinder.fm`, commits
3990ee47 and a845b07f), under the physical cylinder only. Finding N3-1 in [causes.md](causes.md).*

**The base: Gatowski et al.** *Heat Release Analysis of Engine Pressure Data*, SAE 841359 (SAE Transactions 93, 1984,
961-977): γ = 1.392 - 8.13e-5 T for the gas of a single-zone spark-ignition model (fuel-air mixture, residual, and
equilibrium products with their dissociation), the walls taken separately. Quoted from Energies 14 (2021) 3117, which
uses it; the original paper was not opened here.

**The mixture term: frozen products.** Ceviz and Kaymaz (Energy Conversion and Management 46, 2005) fit γ by
temperature and air-fuel ratio; their abstract gives the direction (γ rises as the mixture leans) but their coefficients
were not reachable here. So the size of the lean side's rise was worked from first principles: the complete products of
CH1.8 burned at φ (per carbon: 1 CO2, 0.9 H2O, (1.45/φ - 1.45) O2, 3.773 x 1.45/φ N2), no dissociation, with the
molar heat capacities of JANAF's tables (recalled to 1 to 2 %), and γ = c_p / (c_p - R) of the mixture:

| T, K | c_p N2 / O2 / CO2 / H2O, J/(mol K) | γ, φ 0.7 | γ, φ 1.0 | Lean - stoich | Gatowski | The fit before N3-1 |
|---|---|---|---|---|---|---|
| 1000 | 32.70 / 34.87 / 54.31 / 41.27 | 1.304 | 1.293 | 0.011 | 1.311 | 1.338 |
| 1500 | 34.85 / 36.56 / 58.38 / 46.92 | 1.277 | 1.266 | 0.011 | 1.270 | 1.300 |
| 2000 | 36.00 / 37.74 / 60.35 / 51.18 | 1.264 | 1.253 | 0.011 | 1.229 | 1.263 |
| 2500 | 36.69 / 38.67 / 61.47 / 53.98 | 1.257 | 1.246 | 0.011 | 1.189 | 1.240 (its clamp) |

Read across: the lean gas sits 0.011 above the stoichiometric at every temperature, so 0.011 / 0.3 = **0.037 per unit
of (1 - φ)**, held at 0 rich (the rich side's chemistry is the mixture's heat: N3-2). Gatowski's fit matches the frozen
stoichiometric products at 1500 K and falls below them hotter, which is the dissociation it carries and they do not. The
fit before N3-1 lay above even φ 0.7's products below 2000 K.

To check it (Python, the table's own numbers):

```python
cp = {1000: (32.70, 34.87, 54.31, 41.27), 1500: (34.85, 36.56, 58.38, 46.92),
      2000: (36.00, 37.74, 60.35, 51.18), 2500: (36.69, 38.67, 61.47, 53.98)}   # N2, O2, CO2, H2O
def gamma(phi, T, y=1.8):
    a = (1 + y / 4) / phi                          # O2 supplied a carbon
    n = (3.773 * a, max(a - (1 + y / 4), 0.0), 1.0, y / 2)
    c = sum(k * v for k, v in zip(n, cp[T])) / sum(n)
    return c / (c - 8.314)
```

What it leaves out: the unburned charge's own γ during compression (a diesel compresses air, γ some 0.02 above the
fit there; a petrol charge with its fuel vapour near the fit), and dissociation's dependence on φ.
