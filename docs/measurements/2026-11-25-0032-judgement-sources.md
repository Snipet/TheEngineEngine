# Five judgements left without a source on 24 Nov, looked up

*For the lead's list (00:29 UTC): each figure either cited, with what the source measured, or "not found" with where it was looked for. Kinds as in the EA888 file: maker, paper, catalogue, press, forum (enthusiast); "unverified" where only a search snippet was seen.*

## 1. A variable-geometry turbo's vane position at a warm idle, no EGR demand (D2, N2: 11 to 17 dB of a diesel's idle note)

**Not found as a figure for a warm idle with no EGR demand.** What was found points to the vanes **closed (narrow) at idle**, more so when cold:

| Figure | Kind | Source |
|---|---|---|
| "To allow a quick build-up of charge pressure at low speed and under full load, the vanes are set to a narrow inlet cross-section"; the open (widest) position is the emergency one, the actuator unpowered. Idle not mentioned. | maker | VW SSP 190 (vaglinks.com, VWUSA.COM_SSP_190_TurboCharger.pdf) |
| At curb idle the stock thermal-management calibration "utilizes exhaust manifold pressure control, via an over-closed VGT"; their "Open VGT" case opens it "nearly 33% from its stock position". A medium/heavy-duty six, the warm-up strategy (not the fuel-efficient baseline). | paper | Vos et al. 2019, Frontiers in Mechanical Engineering (doi 10.3389/fmech.2019.00052) |
| "Vane control at idle does what it wants which is closed down all the way" (GM LML Duramax) | forum | duramaxdiesels.com, "idle vane control" |
| "At low idle the commanded VGT % should be around 73 % (for a hot engine), 85 % or so ... cold" (Ford 6.7) | forum, **unverified** (snippet; the pages refused) | ford-trucks.com threads 1164417, 1388746 |

Looked for and not reached: VW SSP 403 and 826803, Ford and GM service manuals, Garrett's and BorgWarner's VNT papers.

## 2. A diesel's cycle-to-cycle spread of combustion timing at idle (D2's clatter: 0.64 deg)

**The 0.64 deg is in the paper, but it is not an idle figure and not the start of combustion.** Szpica and Kluczynski, "An Assessment of the Non-Repeatability of a Diesel Engine Cycle-by-Cycle Operation Under Variable Load and Speed Conditions", Energies 2026, 19, 1343 (paper; read in full from mdpi-res.com's PDF): a Perkins 1104D-E44TA (4.4 L, turbocharged, 90.1 kW at 2200 rpm), 500 cycles a point, the variable the **crank angle of peak cylinder pressure** (whole degrees: about 1 deg resolution).
- 0.64 deg: "standard deviation, sigma = 0.64" at 1000 rpm and 200 N m (sequence 1); 1.16 at 2200 rpm and 400 N m.
- **At idle (800 rpm, no load, its Table A3): mean 2.542 deg, sigma 0.559 deg, range 1 to 4 deg, COV 22 %; COV of peak pressure 1.70 %, of IMEP 6.92 %.**

A start-of-combustion or ignition-delay spread at idle: **not found** (a patent, US 7401504, gives 0.11 to 0.48 deg at 1500 to 3000 rpm for a detection delay, not cycle-to-cycle; SAE 2006-01-3410's abstract says the spread is highest at the lowest speed, no numbers; SAE 790924 and 831352 paywalled).

## 3. A small automotive radial turbine's efficiency against blade-speed ratio, and a mixed-flow one's (N2: 0.52 to 0.62 in the model)

| Figure | Kind | Source |
|---|---|---|
| A conventional radial turbine's highest energy recovery near U/C 0.7 | paper | Rajoo and Martinez-Botas 2008, Int. J. Fluid Machinery and Systems 1(1) (koreascience.or.kr JAKO200820065114349) |
| A mixed-flow turbine (20 deg blade angle, a Holset H3B volute): nozzleless peak 75 % total-to-static at U/C 0.63; with nozzle vanes 79, 80 and 77 % at 60, 65 and 70 deg settings, at U/C 0.62 to 0.68 | paper | the same |
| Radial turbines "provide peak efficiency at speed ratios of 0.7" | paper, **unverified** (abstract in a snippet) | Luddecke, Filsinger and Ehrhard 2012, IJRM, doi 10.1155/2012/589720 |

**A measured peak efficiency for a 40 to 55 mm radial automotive wheel: not found** (Serrano et al. 2017 gives no peak value; Watson and Janota, Baines and Garrett's pages not reached; several publishers refused).

## 4. A radiator fan thermoswitch's on and off temperatures (M: 100 and 95 C, judgement)

| Switch | On / off | Kind | Source |
|---|---|---|---|
| Mahle Behr 0065453924 (Mercedes 190E and others) | 100 / 95 C ("switch-on point = 100 C, shut-off point = 95 C") | catalogue | fcpeuro.com |
| Febi Bilstein / FAE 61311364272 (BMW 318i, 325i) | 95 / 90 C | catalogue | fcpeuro.com |
| VW 1H0959481B, two stages (Golf Mk3/Mk4, Polo, Vento) | 95-84 / 102-91 C (11 C each stage) | catalogue | heritagepartscentre.com |
| Mahle TSW32 (Audi/VW, three-pin) | 87-76 / 93-82 C | catalogue | fcpeuro.com |

**So M's 100 / 95 C is cited**: it is the Mercedes Behr switch's exactly; the spread runs 5 C (Mercedes, BMW) to 11 C (VW, Audi). No Japanese pair found.

## 5. A 2 L petrol engine's warm-up from 20 to 90 C at idle (D: 4.8 min in the model; "8" in a comment)

**Not found** as a logged idle figure for a 2 L petrol engine. The nearest: a VW Golf 4 1.6 L petrol logged over OBD at idle, "all repetitions during idle mode were carried out for 15 minutes ... at ambient temperature of 24 C", its charts from 25 to 80 C, no idle speed and no time to 90 C (paper: Engineering for Rural Development 2016, iitf.lbtu.lv N086.pdf). Driving warm-ups (not idle) on a seatcupra.net thread (forum): a 2.0 TDI "5 miles" to 90 C at 5 C outside. Looked for and not reached: briskoda.net, a Polish idle warm-up paper (yadda.icm.edu.pl), SAE 900683 (a 2.5 L simulation, no idle case).

*Read 25 Nov, 00:45 UTC at D's request: N086 (Birkavs and Dukulis, ERD 2016, pp. 467-472) has **no temperature-against-time curve**: its Figs. 2 to 6 are bar charts of CO, CO2, HC, O2 and lambda against coolant temperature in 5 C bins from 25 to 80 C (idling against driving); it says it recorded "the time in which the engine of the vehicle reaches the operating temperature" but does not report it. Three idle runs of 15 min each at 24 C, each after 23 h standing; a 66 kW 1.6 L, no engine code, no idle speed. That its bins reach 80 C suggests about 80 C within 15 min at idle: an inference, not a figure. It does not settle the model's 4.8 min.*


## 6. The residual gas fraction at idle with large valve overlap (the lead, 00:50: the V10, V12 and 7.0 read 0.47 to 0.53 at intake closing)

**Fox, Cheng and Heywood, SAE 931025: the original not reached (SAE Mobilus, paywalled).** Reprinted in Albert, "Residual Gas Effects on Combustion in an Air-Cooled Utility Engine", MS thesis, U. Wisconsin-Madison 2004 (thesis; minds.wisc.edu), Eq. (2.2), p. 14:
- y_r = 1.266 (OF/N · P_int/P_exh)^-0.87 · sqrt|P_exh - P_int| + 0.632 (phi/r_c)(P_int/P_exh)^-0.74. The constants and exponents are certain. The square root is a reading of a garbled PDF layout, not a clean quote. "N is the engine speed, OF is the overlap factor..., phi the equivalence ratio, r_c the compression ratio."
- OF (Eq. 2.3) = [D_int ∫(IVO to the lifts' crossing) L_int dθ + D_exh ∫(the crossing to EVC) L_exh dθ] / V_dis. "D_int and D_exh are the intake and exhaust inner valve seat diameters, L_int and L_exh the lift..., V_dis the displaced volume". So it uses the inner seat diameter and the actual lift curve.
- **Units:** OF in deg/m (Albert's Table 4.2, p. 45). N's unit is not tied to rpm or rev/s, no pressure unit is given, and the constant's units are not stated. Albert doesn't say whether V_dis is per cylinder or for the engine. **The units question (N3's 1/30 of an orifice) is not settled by this source.**
- A related, unfitted form, in full (Maldonado and Kaul, Int. J. Engine Res. 23(2) 2022; the OSTI copy (osti.gov 1756265) printed page 4, checked against a 300 dpi render; the nomenclature pp. 13-14; paper), with theta = 370 CAD:
  - **Eq. 7:** X_res,VO[k] = sqrt(1/C[k]) · ((r_c - 1)/r_c) · sqrt(R · T_int · |E_theta[k] - I_theta[k]| / E_theta[k]) · (π√2/6) · (OF/N) · (E_theta[k] / I_theta[k])^((γ+1)/(2γ)). The first root covers 1/C[k] alone, the second R T_int |E - I| / E; (r_c - 1)/r_c is under neither. "Here, N is the engine speed and OF is the overlap factor"; C[k] is "calculated using Eqn. (3)" and is not in the nomenclature. X_res = X_res,IVO + X_res,VO (Eq. 9).
  - **Eq. 8:** OF = [Z_int · D_int · ∫(IVO to theta) L_int dθ + Z_exh · D_exh · ∫(theta to EVC) L_exh dθ] / V_d, "where L is the valve lift, D is the valve inner seat diameter, and Z is the number of valves". I_theta and E_theta are "the intake and exhaust manifold pressures ... recorded by the high-speed sensors at the crank angle theta for cycle k".
  - **Units as the paper lists them:** D valve inner seat diameter [m]; E, I high-speed exhaust and intake manifold pressures [Pa]; L valve lift [m]; **N engine speed [RPM]**; OF overlap factor [-]; R gas constant for dry air [J/kgK]; r_c [-]; T_int intake manifold temperature [K]; V_d displacement volume [m3]; Z number of valves [-]; gamma [-]; X_res [-]; theta, IVO and EVC (crank angle) given no unit.
  - **The paper's units do not close:** by them, Eq. 8 leaves degrees (m · m · m·deg / m3), so OF is deg (or rad) where it is listed [-], and Eq. 7 with N in rpm is not dimensionless as written. A dimensional check must choose the angle's unit (deg or rad) and N's (rpm or rad/s) and say which; the paper does not.
- **The data's range:** "0.52 < OF < 1.43" (Albert p. 56, citing Fox et al.). The speed and manifold-pressure ranges were not found.
- Heywood's 2nd edition page: not found.

**Measured residual fraction against overlap:**
- Heywood 1st edition (1988), Fig. 6-19, pp. 230-231 (textbook; a readable copy with values not found on 25 Nov: not in MIT OCW 2.61's notes, nor a Wisconsin thesis; a Nottingham thesis (eprints.nottingham.ac.uk/10821) has a section on overlap and residual but its PDF did not download): a 2 dm³ four (88.5 mm bore; Toda et al., SAE 760765), residual by in-cylinder CO2 against manifold pressure, for several speeds, compression ratios and overlaps, at 1400 rpm, A/F 14.5 and CR 8.5; its problem 6-7 cites "27° valve overlap". **The curves' values could not be read from the text copy, and how the overlap is defined was not found.**
- Albert 2004 (thesis): an air-cooled single, residual by in-cylinder sampling and NDIR CO2. Overlaps 0, 50, 90 and 130 deg **at 0.25 mm lift**, OF 0.11, 3.66, 12.31 and 27.26 deg/m. Speeds 1200, 1750 and 3060 rpm, at WOT, 25 % and 10 % load. **Measured y_r up to about 0.35 to 0.40**, the most at the largest overlap, lowest speed and 10 % load (read off Figs. 4.4 and 4.6, pp. 51-53, not quoted). "The combustion stability was found to be sensitive to increasing amounts of overlap." Fox et al. "significantly over-predicts the residual fraction for all of the valve overlap conditions except for the zero overlap configuration" (p. 56, Fig. 4.9). Note that its OFs run far beyond Fox's 0.52 to 1.43.
- Idle misfire or residual measured on a long-duration cam at idle in a multi-cylinder engine: **not found.**

## 7. Reverse-flow discharge coefficients at low lift (D's ask, 25 Nov 01:04 UTC)

Asked: Cd against L/D 0.01-0.1 for the exhaust valve flowing port to cylinder and the intake valve flowing cylinder to port, with the source's reference area and pressure ratio. **No measured value found in an open source, for either valve or direction.** Where it looked:
- Blair, *Design and Simulation of Four-Stroke Engines* (SAE 1999), ch. 3 (book): no open copy. The data behind it is Blair & Drouin, SAE 962527 (1996, paywalled): a 600 cm³ four-valve motorcycle head, steady flow with air, both valves, inflow and outflow (abstract snippets). Blair, Callender & Mackey, SAE 2001-01-1798 (paywalled): inflow and outflow maps against area and pressure ratio up to 2.0 (search snippet, page not opened). **Blair's measuring both directions is confirmed by these abstracts; the values are not.**
- Blair, "Back to basics" (magazine article, profblairandassociates.com), Fig. 9: exhaust **outflow** (forward) only, Cd against an undefined "lift ratio" 0-1 and pressure ratio 1-2; reference area the side of a frustum of a cone on the seat diameters. A 3-D surface; no values readable.
- SAE 2021-36-0107 (paywalled), whose title names the intake port's reversal Cd: abstract ranges only, 0.45-0.91 exhaust and 0.42-0.69 intake over 0-7.3 and 0-8.2 mm lift, no diameter, reference area or ΔP.
- Woods & Khan, Proc. IMechE 180 (1965-66), via a 2017 Exp. Fluids snippet: below ℓ/d 0.04 the effective area is independent of pressure ratio; direction not given, no Cd.
- Kastner, Williams & White (Proc. IMechE 1963), Heywood Fig. 6-21, Annand & Roe, Bicen, Vafidis & Whitelaw: no open copy. ENCIT2012-158: forward intake only.
**Next:** SAE 962527 or 2001-01-1798, or Blair ch. 3, from a library. Until then any reverse Cd is judgement.

## 8. Cam flanks: the seat-to-1 mm step (the lead's ask, 25 Nov 01:10 UTC)

Our cards' second duration sits 13 to 16 crank deg a side inside the seat's on every overhead-cam archetype (the V10 and V12 13 to 14), all generator judgement. Durations below are crank deg, Q quoted, C computed (seat minus second, halved). Downloads 25 Nov.

**Catalogue cards, pushrod** (tappet lift 0.006 in to 0.050 in; at the valve, through a 1.5 to 1.7 rocker, about 0.25 to 2 mm, **not** our 0.15 to 1 mm):
- Hydraulic flat tappet: COMP XE268H-10 268/280 Q, 224/230 Q @0.050 (montmotorsports.com), 22/25 C; COMP CRS 268H-10 268 and 218, 25 C (manciniracing.com); Howards 110041-10 and 110051-12, 27 C (howardscams.com); Crower 00238 250/258 and 192/196, 29/31 C (crower.com card); Erson E110044 292 @0.006 in Q and 230, 31 C (enginelabs.com). Range 22 to 31 a side (44 to 62 in total).
- Hydraulic roller: COMP XR276/282/288HR, 26 a side C, the 0.006 in quoted (streetmusclemag.com); Crower 00571/2/3 and 00462/5, 23 to 29.5 C (crower.com). Range 23 to 29.5.
- Solid roller (lash 0.026 in and up): Crower 00425/00429/00432, Howards 110013-08: 14 to 20 a side C, checking height not stated.
- Advertised heights are not stated by Crower or Howards; "anything from .004-inch to .022-inch" historically (enginelabs.com).

**Overhead-cam four-valve grinds, metric heights:**
- Kelford spec sheets (kelfordcams.com/wp-content/uploads/product-specs/<PN>.pdf), "Advertised Duration @ 0.10mm" and "Duration @ 1.00mm VALVE LIFT" Q: 46 sheets at 0.10 mm. Buckets, 40 lobes, 16.5 to 31 a side, **median 20.0** C (e.g. 1GR-FE 256/222, 17; 1FZ-FE 264/225, 19.5; 2AZ-FE 264/218, 23). Fingers and rockers, 52 lobes, 14 to 32, **median 21.0** C (e.g. B58 270/230, 20; K20C4 264/224, 20; EcoBoost 2.3 262/220, 21; B16A 276/248, 14).
- Kent Cams (kentcams.com, 2019 brochure: OHC durations at 0.25 mm, "Duration @ 1mm" with a 1 mm clearance): Zetec and Duratec grinds 13 to 21.5 a side C from 0.25 mm.
- Web Cam's "duration" has no stated height (9 to 14 a side C): not used.

**Production DOHC engines:** no manual or paper found giving timing at both the seat and 1 mm, and no lift table for a named engine (manuals give one height, Europe's mostly 1 mm). Brian Crower's 2010 catalogue lists stock cams' "ADV" (height not stated) and @0.050 in: Toyota, Nissan, Subaru and Mitsubishi DOHC 18 to 28.5 a side C; Honda's 30 to 51 (its "ADV" plainly at a much lower height). Not used for the rule.

**Class rule** (written before any table; basis judgement within cited brackets): the 0.15 to 1 mm step lies inside Kelford's 0.10 to 1 mm (medians 20 and 21) and is no less than Kent's 0.25 to 1 mm (13 to 21.5). **Bucket 18 deg a side; finger follower and rocker 19 deg a side**; the seat timing kept, the 1 mm duration = seat − 2 × step. **Pushrod:** no source gives valve-lift 0.15 and 1 mm; the four pushrod cards carry a 0.050 in tappet figure (about 2 mm at the valve) in the 1 mm field, so their 1 mm duration is **too short**, not too long: judgement 20 a side for hydraulic tappets (inside the cited 22 to 31 from the larger 0.006 to 0.050 in span), taking the seat as stated. 
**Archetypes with a source of their own:** the turbo four's exhaust (180 and 215 at 1 mm, Q, SSP 922903; seat by the rule, 218 and 253); the two-stage's B stage seat (280/278, aggregator); the 454's @0.050 in (213/217, its seat a forum post); the L48's (from memory). Every other card, and every missing figure of these, takes the class rule.

## 9. Warm-idle fuel rates by engine size (N1's ask for the lead, 25 Nov 01:37 UTC)

Q quoted, F read off a figure, D derived. Measured, light-duty, accessories off where stated:

| Vehicle, engine | Idle fuel | Basis | Conditions | Source |
|---|---|---|---|---|
| Ford Focus, 2.0 L petrol | 0.16 gal/h (0.61 L/h) "no load"; 0.29 with load | Q | dyno (APRF); "no load" = no AC or fans per DOE Fact #861; gear, rpm, warmth not stated | Argonne Idling Reduction Savings Calculator, reverse table (ANL 1: Stutenberg & Lohse-Busch 2012); ORNL/TM-2016/224 pp. 25-27 |
| VW Jetta, 2.0 L diesel | 0.17 gal/h no load; 0.39 with load | Q | as above | same |
| Ford Crown Victoria, 4.6 L V8 | 0.39 gal/h (1.48 L/h) no load; 0.59 with load | Q | as above | same (ANL 2: Rask et al. 2013, not found) |
| 2011 Ford Fusion, 2.5 L I4 | 0.279 cc/s = 1.00 L/h | Q (D in L/h) | warm (after catalyst light-off), 21 °C, flow meter; AC and gear not stated | Gaines, Rask & Keller, "Which Is Greener", Table 1 (afdc.energy.gov) |
| Lancia Ypsilon, 1.24 L I4 NA | 0.60 L/h | F | +25 °C, coolant 85-95 °C, "heating, air conditioning, lighting etc. were off", minimum idle | Adamski & Polak, Combustion Engines 2017 169(2) Fig. 8 |
| Fiat Doblo, 1.6 L turbodiesel | 0.58 L/h | F | as above, coolant only 60-70 °C | same |
| 2009 Ford Explorer, 4.0 L V6 | about 0.36 gal/h AC off | D (from "~0.2 gal/h ... a 55% increase") | dyno | ORNL/TM-2016/275 (SAE 2013-01-0551) |
| 2009 Toyota Corolla, 1.8 L I4 | about 0.22 gal/h AC off | D (from "~0.13 gal/h ... 60% increase") | dyno | same |

**Fitted lines, not measurements:** Argonne's light-duty worksheet, 1 to 5 L: 0.16, 0.32, 0.48, 0.63, 0.79 gal/h "no accessories" (Q), no data basis stated; it is NRCan's "0.6 litres/hr per litre of engine displacement" (Q; Taylor 2003, not found) to the rounding. Argonne's own measured range: "0.2 to 0.5 gal/h for passenger vehicles across a range of sizes, and increased with idling speed" (Q). **The line over-predicts every measured warm idle above 2 L by 25 to 45 %**, and the measured rates grow less than in proportion to displacement. No source states neutral or park; none gives the idle speed.
