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

## 10. Manifold pressure at warm idle (the lead's ask, 25 Nov 01:46 UTC)

kPa absolute, 1 inHg 3.386 kPa, vacuum taken from 101.3 kPa unless stated. Q quoted, D derived.

| Engine | Idle MAP | Idle speed, conditions | Basis | Source |
|---|---|---|---|---|
| Honda L15 (Fit/Jazz), the economy 1.5's class | 20-34 | 750 ± 50 rpm (L15A1/A6 page); warm, CVT in P or N, M/T in N, A/C and all accessories off | maker, Q: "At idle speed: about 20-34 kPa (150-260 mmHg, 6-10 in.Hg)" | Honda workshop manual, ECM/PCM data table (hondziarze.duckdns.org/jazz) |
| Toyota 4A-GE 16V | about 48 (400 mmHg vacuum); or 53.5 as the post writes it | 800 rpm; conditions not stated | a forum post quoting the maker, Q: "Toyota 16V 4AGE Service Specification of 53.5 kPa (abs) = 15.75" Hg @ 800 rpm idle", from the May 1987 4A-F/4A-GE repair manual, p. A6 (the manual not read; 15.75 inHg is 400 mmHg of vacuum, so the post's "abs" is likely wrong) | aeu86.org thread "vacuum reading on idle" |
| GM LS7 (stock) | 38 | not stated | forum, Q: "The stock ls7 idles at 38 kPa" | forum.hptuners.com, p=322472 |
| BMW S54 | about 30 (21 inHg) | not stated | forum, second-hand, a search snippet only; the S54 has no MAP sensor (BMW ST041: hot-film air mass meter, idle air actuator) | bimmerforums t-2235462; BMW ST041 |
| Audi 5.2 V10 FSI; Lamborghini L539, Ferrari F140 and other NA V12s | not found | | | Ross-Tech wiki, Audizine, AudiWorld (blocked), FerrariChat, MoTeC guides |

**Class figures:** MOTOR magazine (July 2006), Q: "On a normal engine, normal idle vacuum should be a steady reading between 17 and 21 in./Hg", warm: 30 to 44 kPa. AA1Car (L. Carley), Q: "typically ranges from 16 to 20 inches Hg in most vehicles": 34 to 47 kPa. Toyota 1AZ-FSE "Idling: 22 - 32 kPa" and Suzuki SX4 "24 - 38 kPa at specified idle after warm-up" (maker data lists, search snippets only). Cams (trade and forum, snippets): overlap lowers idle vacuum by reversion; stock engines 30 to 40 kPa, cammed "from 50 on up"; a cammed LS7 at 67 kPa.

**As validation rows** ("Idle manifold pressure", p-idle-warm, 8 to 14 s): the economy four 20-34 (cited, the class's maker); the hot hatch 43-54 (recalled: the forum's quote of Toyota, both readings, plus 5 kPa judgement); the 7.0, 3.2, V10 and V12 30-44 (recalled: MOTOR's class figure, their own not found or forum only).

## 11. Valve timing at a stated lift (the lead's ask, 25 Nov 01:42 UTC)

**No maker's source found for these five gives the lift height its valve events are quoted at.** M maker, E enthusiast reference, F forum.
- **5.2 V10 FSI** (Audi SSP 923603 / 376, M): both cams phased, "The adjustment range of the camshafts is 42°" (unit not stated); "mechanically locked into place until sufficient oil pressure is developed", lock position not stated; exhaust "210°". Its parent 4.2 V8 FSI (SSP 377, M): "Adjustment range of the camshafts is 42 crank angle degrees"; "locked ... at a standstill; Intake in advance position, exhaust in retard position"; intake 200, exhaust 210 crank degrees (230/220 on the RS4's); "internal exhaust gas recirculation through the use of a corresponding valve overlap". No events found.
- **L539 V12** (Aventador press release, Lamborghini bulletin BI.07.17, M): intake and exhaust phased; no angles. F140: nothing.
- **4A-GE** (E, F; no Toyota document found): big-port IVO 9 BTDC, IVC 51 ABDC, EVO 51 BBDC, EVC 9 ATDC, 240 deg, 7.56 mm, centres 111/111; "240 gross duration, duration @.050" lift is 204" (F). Small-port 232 deg, IVO 8 BTDC, IVC 44 ABDC, exhaust 44/8 or 47/5, 7.1 mm, centres 108/111 (F); lift height of the events not stated ("approximate", Club4AG).
- **S54** (BMW ST041 and ST609, M): lift 12/12 mm; spread (lobe centre) intake 70-130, exhaust 83-128 crank degrees; ranges 60 and 45; "The 'default' mechanical stop position without VANOS influence is: Intake Camshaft = Retarded (130º spread angle) / Exhaust Camshaft = Advance (83º spread angle)"; a row 260/260 (probably duration, read as such). Idle position not found. **Which end "Advance (83º)" is**: no second BMW line found (the workshop timing pages, workshop-manuals.com S54 11 31, pp. 631-632, give no spreads, only "The VANOS adjustment travel on the exhaust side is less than that on the inlet side"; Beisan Systems' procedure takes both cams to "fully retard" for removal, no number). The model's evidence (N1, 25 Nov 01:58, warm idle on the S54 card): exhaust at rest at 128, overlap 2 deg at the seat, 850 rpm, sd 3.0, runner pressure 37.1 kPa, burned share 0.232; at 83, overlap 47 deg, sd 5.1, 60.4 kPa, share 0.501. The card rests it at 128 (the lead's reading).
- **1NZ-FE** (a Toyota-format table on iFixit, M?): IVO -7 to 33 BTDC, IVC 52 to 12 ABDC (40 deg range, 225 deg), EVO 42 BBDC, EVC 2 ATDC (224 deg); at full retard about -5 deg overlap (D). Locks at most retarded (1NZ-FXE repair manual EM-152, M; toyota-club.net, E: idle at "maximum retard", "Minimal valves overlap").

**Against the cards** (seat; centres after the intake's park): the S54's cited parks are intake 130, exhaust 83 lobe centre; ours park the intake at 126 and fix the exhaust at 110. The 4A-GE's 240 deg on 111/111 (gross) against ours 256/248 on 108/110. Every other centreline and park in the five cards is judgement.

## 12. Cylinder head flow sheets; the F20C's changeover (the lead's asks, 25 Nov 02:02-02:06 UTC)

**GM LS7 head, production (GM CNC-ported at the factory, not as cast)**, measured third party, RLD Performance (rldperformance.com/general-motors/gm-heads/ls7, the "Stock GM CNC LS7 Ports" columns): "All airflow data is corrected data (Temperature and Atmospheric Pressure) at 28" of water. Intake is measured with a shaped entry and the exhaust is measured with a stub pipe." "Valve= 2.20 1.61" (stated once on the page, for the stock or the ported columns not said); chamber 70 cc, runner 270 cc; bore fixture and bench not stated. Intake / exhaust CFM at 0.100 to 0.700 in: 71/50, 145/120, 222/158, 271/192, 315/207, 332/214 (0.550), 348/219, 350/220 (0.650), 352/221. Forum figures for the stock head (corvetteforum t-1252849, search snippets only, no depression stated) run 5 to 10 % higher at 0.300 to 0.600 in.

**Production four-valve heads as cast (4A-GE, B16A, K20A): not found with stated conditions.** A K20A2 intake-only sheet, "bone-stock head with stock valves", Superflow 600 at 28 in, corrected (k20a.org thread 10689, a search snippet only, pay gate): 107.8, 196.5, 261.7, 294.7, 307.8, 316.6 CFM at 0.100 to 0.600 in; no exhaust, no valve sizes. **Two-valve wedge:** an 882 small block (1.94/1.50, 76 cc), a forum member's figures "at 28" H2O" (search snippet, page 403): 70/58, 125/108, 175/135, 204/141, 205/142, 206/142 at 0.100 to 0.600 in. 454 oval port: not found. Stan Weiss's compiled tables: not reachable.

**F20C VTEC changeover: 5850 rpm, the maker's.** Honda's 1999 S2000 fact book (honda.co.jp/factbook/auto/s2000/199904/013.html): 「5,850rpm～9,000rpmという広い回転域をハイバルブタイミング域に設定し」; global.honda's F20C page the same; American Honda's 2001 powertrain release: "allowing the valvetrain changeover point to be set at a relatively low 5850 rpm", and its generic "At around 6000 rpm (depending on throttle position)" (the F22C1's releases only the latter). The card takes 5850 for table L (the lead, 02:06). The F20C's own lobes: an aggregator (s2000.club, a mywikimotors print) gives "300/298 deg, lift 12.65/11.66 mm" with no height, lobe or source named: found and unusable; no maker's or tuner's stock row found.

**Held-stage dyno runs (low cam only, high cam only) for a B16, K20 or F20C: not found as numbers or a readable chart** (the forums that would hold them, S2KI, Honda-Tech, k20a.org, ClubRSX, MotoIQ, return 403 or a pay gate). What exists: Hondata's tuning help (hondata.com/help/flashpro/tuning_vtec_point.htm, a tuner's, Q): two runs, "one with VTEC set low (e.g. 3000 rpm)" and one high, "Set the VTEC point to the intersection of the high speed cam and the low speed cam"; "Normally there will be a small dip (less than 5 lbs torque) at VTEC"; typical points "stock engine 5500-5700 rpm", intake and exhaust mods 4500-4800, supercharged 3000-3200; "Not recommended to run over 7000 rpm on the low speed cam". An S2000 AP1 owner (S2KI, a search snippet): "every attempt at timing/fuel brings the torque curve intersection to 5800-5900" (forum). So on a stock engine of the kind the two stages cross near the factory switch, within some 300 rpm (a tuner's and a forum's word, no curve).

**B16A cam timing (for the sources only; the lead, 02:12).** No open and close angles for any stage found. Honda's VTEC history page (honda.co.jp/sportscar/vtec_history/B16A, maker): the low-speed intake closing 20 / 30 deg ABDC (an earlier cam 35), the lobe and height not stated. Honda Global's B16A page (maker): "Switching occurs around the 4,800-5,200 rpm range." Stock high (VTEC, "mid") lobes, tuners' catalogues: Brian Crower 2006 (briancrower.com/dl/06cat.pdf), USDM B16A2 advertised 280/276, @0.050 in 219/214, lift 10.59/9.47 mm; Crower's master catalogue (crower.com, Honda_Acura page 1; "Duration figures are taken at the valve", advertised at "(.010")") Civic Si (B16A2) 284/282, @0.050 224/220, lift 0.412/0.379 in; the B16B 294/286-305, 233-240/224-232. Stock low lobes (Crower, engine not named): secondary 278/288 advertised, 200/201 @0.050, 0.321/0.306 in; primary 259/230, 176/179, 0.204/0.190 in. Rocker ratios mid 1.55, secondary 1.50, primary 1.45. Crower's table is scrambled: its intake/exhaust split is read by matching Brian Crower's rows. The two catalogues differ by up to 5 deg at 0.050 in and 14 to 19 deg advertised.

## 13. The two-stage engine's intake runner: F20C and K20A Type R (the lead's ask, 25 Nov 02:17 UTC)

**No maker's runner length, diameter or plenum volume found for either.** Honda's 1999 S2000 fact book (honda.co.jp/factbook/auto/s2000/199904/016.html, maker): a new "ストレートポートインテークマニホールド" with "ポート径とポート長、チャンバー容量を縮小", the volume after the throttle cut "従来の1.6リッタークラスエンジンレベルまで"; single, fixed length; no figures. Honda's 2001 Integra fact book (honda.co.jp/factbook/auto/INTEGRA/200107/06.html, 07.html, maker): the Type R's "単管等長ショートインテークマニホールド", "単管ショート式シングル" (the iS a rotary-valve dual), throttle bore 62 mm, air intake 70 mm. Honda R&D Technical Review 12-2 (2000), the lean-burn K20A (not the Type R): runners of "540mm and 270mm" by a rotary valve; "the former model's intake runner length of about 300mm is adequate at high revolutions" (maker, another engine).
Forums and an owner's blog only: the S2000 manifold "about 15.5 cm or about 6 inches" (racings2000.blogspot.com, an owner, probably the AP2's); "OEM runners are 7.5"" (S2KI, a snippet); the AP1 runner 51.7 mm at the head flange (S2KI, measured by a member, a snippet); K20 manifolds PRB about 8.25 in and RBC about 7 in or 190 mm, disputed (k20a.org, ClubRSX, snippets); runner exits 50.5-51.1 mm. Plenum volumes, bellmouths and head port lengths: not found.
**So the card keeps the sports class's 0.30 m runner, judgement as marked** (the forum figures, 155 to 190 mm from the flange, would add the head's port, not found).

## 14. Heavy diesel: the transient smoke limit and the time to boost (the lead's ask, 25 Nov 03:01 UTC)

**Smoke limit while boost builds.** No maker's published smoke-map setting, service manual or aneroid setting found. What exists:
- Cummins patent US 6,508,241 B2 (maker, a patent's example): a transient fuel limiter capping fuel from a table of maximum oxygen-based equivalence ratio (O2 in the cylinder, the EGR's included) against speed and commanded fuel; FIG. 7's y-axis runs about 0.73 to 0.79 (λ about 1.27 to 1.37), the axis ticks, not curve values.
- Clean Air Power patent US 7,270,089 B2 (maker, a patent): a diesel's smoke limit "typically about 1.3" (λ), in a turbocharged heavy-duty engine with EGR.
- Detroit Diesel patent US 7,063,076 (maker, qualitative): the transient minimum AFR is higher than the steady one, "the AFR suitable for steady-state operation is usually too low to be effective in controlling the resulting smoke spikes".
- Wahlström, Eriksson, Nielsen, Pettersson (IFAC 2005, with Scania): a smoke limiter on a λmin parameter, its value in a plot only.
- Bosch's diesel fuel injection handbook (1994 or 1996, quoted on tdiclub.com, not read): at the full-load smoke limit "direct-injection engines have an air excess of 40...50%" (λ 1.4 to 1.5, steady, not heavy-duty specific).
So the builds' φ 0.80 (λ 1.25) sits at the rich edge of the patents' 1.27 to 1.37.

**Time to boost or rated torque.** Detroit DD15 (14.8 L), the maker's spec sheet (penskeanz.com, PDF read): "90% peak torque in 1.5 seconds", "At just over 1100 RPM, you're at 90% peak torque"; TRUCKiNG Info (2007, the maker's figures repeated): "four seconds on the S60, where it is only 1.5 seconds for the new engine", torque response defined as "the time the engine takes from mashing the throttle to reaching its torque output peak"; full boost "in about 1.5 seconds, compared to three or four seconds on other engines"; a Detroit brochure: competitors "up to 4.4 seconds" (snippet). A medium-duty turbo diesel's lag "12 engine cycles or 1.17 s" (Giakoumis, Frontiers 2016, a review). Not found: any ISX15/X15, D13, Scania, C15 or OM471 figure in seconds; any SAE load-step test on a 15 L engine. So the class's maker's figures are 1.5 s (a modern engine, the maker's claim) to about 4 s (the older ones), against the model truck's some 20 s.

## 15. 15 L truck diesels' rated torque curves (the lead's ask, 25 Nov 03:13 UTC)

Makers' documents; values at 1100-1300 rpm read off their charts (no numeric tables), printed figures marked "printed".
- **Detroit DD15 455 bhp @ 1625 / 1550 lb-ft @ 975** (brochure DDC-EMC-BRO-0192-0820, 2020, a mirror at xr793.com): flat at 1550 from about 975 to 1500 rpm; 1550 at 1100, 1200, 1300 (read, about 20 lb-ft); printed 1471 lb-ft at 1625. Its ratings table: every torque "@ 975 RPM" (1550 to 1750 lb-ft).
- **DD15 455 bhp @ 1800 / 1550 lb-ft @ 1100** (spec sheet DDC-EMC-FLY-0001-1011): plateau about 1000 to 1400-1500 rpm; 1550, 1550, 1545 at 1100, 1200, 1300 (read); printed 1328 at 1800. Its table: every torque "@ 1100 RPM".
- **DD15 Gen 5 505 bhp / 1850 lb-ft** (spec sheet DDC-EMC-FLY-0121-1121, freightliner.com): peaked, printed 1850 at about 1100 rpm; about 1820 at 1200 and 1300 (read).
- **Cummins X15 Efficiency Series** (Bulletin 5410582, mart.cummins.com): "peak torque starting at 950 rpm"; 500 hp / 1850 flat about 950 to 1400 rpm, 450 / 1750 about 950 to 1300-1350, 400 / 1650 about 950 to 1300; each flat through 1100-1300 (read).
- **Cummins ISX15 560 / 1850** (Bulletin 4971488): "runs from 1150 rpm to 1500 rpm at the full 1850 lb-ft of peak torque"; its table's ratings "@ 1200"; the 2013 ISX15 450 "1650 (2237) @ 1000".
So at 1100 to 1300 rpm an engine of the class makes 0.98 to 1.00 of its peak, its plateau starting at 950 to 1150 rpm. Rows (report.py, the tractor): the least torque from 1100 to 1300 over the peak, at least 0.95; where the torque first reaches 0.98 of its peak, 900 to 1200 rpm (the margins judgement).

**The tractor's rows read for the record** (the lead, 04:07), the archetype as committed, report.py's measures:

| | before (tools 164009a1, 25 Nov 03:15) | after N2's map and sizing (tools 9d55f503, 04:15) | target |
|---|---|---|---|
| predicted peak torque | 2310 N m at about 1516 rpm | 2253 at about 1363 | |
| least torque 1100-1300 over the peak (predicted curve) | 0.64 | 0.735, CORRECTED to **0.98** (below) | 0.95 to 1.0 |
| where it first reaches 0.98 of the peak | about 1476 rpm | about 1204, CORRECTED to **about 1100** (below) | 900 to 1200 |
| take-up at 1100 held, to 90 % of the peak | not reached (34 s) | 7.86 s | 0 to 5 s |
| beside it: to 90 % of the settled | 0.42 s | 6.21 s | |
| beside it: settled at 1100 held, over the peak | 1495 N m, 0.65 | 2149 N m, 0.95 | |

Note: held at 1100 rpm for 34 s the engine settles at 2149 N m, where the predicted curve (tee-dyno --predict, its own settle) reads some 1435 at 1058 rpm and 2243 at 1211: the curve's short settle under-reads the lug while the boost is still rising, so the lug rows read lower than the held test. The predicted curve also dips to some 650 N m at 905 rpm (1056 at 600, 1435 at 1058) before and after. **Correction (card C, 04:24):** the predictor matches 30 s holds within 2.3 % at 1000, 1100 and 1300 rpm on the tip; what is real is the engine's own spool knife-edge near 1060 rpm (held 30 s: 1544 N m at 1058, about 2000 at 1061), and the two lug rows had interpolated across it from the curve's point at 1058. They now read the curve's own points from 1100 to 1300 rpm (2211 to 2258 N m): the least over the peak **0.98**, the plateau reached by **about 1100** (its first point; the edge near 1060 is not resolved by them: "spool edge"). The 0.735 and 1204 above were that artefact. The dip near 905 rpm holds after 30 s (card C; the lead is routing it).

## 16. The 13B-REW's sequential turbos: when the second joins (the lead's ask, 25 Nov 04:08 UTC)

**No Mazda text with numbers read directly.** Mazda's 1994 RX-7 repair manual text (a third-party copy, autorepairshq.com; a search snippet, the page 503): "To prevent a drop of boost pressure when the secondary turbocharger begins to operate, the secondary turbocharger is made to spin prior to its operation"; solenoids for turbo precontrol, turbo control, wastegate control, charge control, charge relief. Mazda's patent US 5,197,287 (maker): the secondary's pre-rotation is triggered by boost pressure and airflow, not speed, and also by a throttle opening increased "by an amount larger than a predetermined amount" (thresholds as symbols only). SAE 941030 (Tashima, Okimoto et al., Mazda, 1994): abstract without figures, the paper paywalled.
**The figures:** an rx7club.com post quoting "the 1993 RX-7 Workshop Manual" (a search snippet, 403; unverified): primary only "under 3000 rpm" at light load, the secondary spun "prior to its operation (3000-4500 rpm)", joining "at about 4500 rpm". Kevin Kelleher's measurements (robrobinette.com/boost_note.htm, an enthusiast's, read): "After twin [turbo] mode kicks in at 4500 at WOT"; "locked in twin mode" released "by a time factor, or by dropping below about 3300 rpm"; "the manual implies twin mode will kick in over 5500 rpm with lite load history". rx-7.wikidot.com (enthusiast): "Below 4,500 rpm, only one turbo is connected". Adaptronic (a tuner's ECU notes): its values "4400 RPM and 1200 ms", with minimum MAP and TPS to arm it. Haltech: the switch speeds user settings, no defaults.
So: the join at about 4500 rpm at full throttle (an enthusiast's measurement and a forum's quote of the manual), pre-spool from about 3000 (the forum's quote only), release below about 3300.

## 17. A 15 L truck diesel's BSFC, lambda, boost and turbine inlet (D2's ask with the fuel curve, 25 Nov 05:03 UTC)

**BSFC.** WVU for the ICCT (2018, theicct.org ICCT_CST_US_EU_HDV_FinalReport_20180515.pdf; measured third party): a best-in-class US tractor's engine, anonymised as a 14 to 15 L six, 298-315 kW, 2170-2440 N m, "Amplified Common Rail System, Asymmetric Turbocharger" (a DD15 by its description, an inference), on a chassis dyno at steady state with the fuel weighed and the ECU's work: at 100 % load 177.1, 178.8, 179.5, 177.8, 181.1, 200.4 g/kWh at 800, 1000, 1200, 1400, 1600, 1800 rpm (Fig. 3.5); on the ECU's fuel 186.8 to 208.3 (Fig. 3.6); "highest engine thermal efficiency is found to be ~46%"; the test fuel's 42.79 MJ/kg. The ICCT's certification survey (Delgado & Lutsey 2015): "BTE as evaluated at peak torque for certified diesel engines ... most engines are 43-44% BTE" in 2014, 41 % in 2012 (191 to 205 g/kWh at 42.8 MJ/kg); the DOE SuperTruck baseline "42%". A Cummins research ISX15 (OSTI 1474075): 179.8 g/kWh at 1130 rpm, 1437 N m. Not found: Mercedes' OM471 "best point" figure, any X15, D13, MX-13 or C15 g/kWh.
**Lambda and turbine inlet.** A Volvo D13 520 hp study (Chalmers MSc 2011 with Volvo Powertrain, design constraints, not measurements): "a minimum of lambda 1.25 is maintained"; "Exhaust temperature Lower than 950 K", "Exhaust pressure Lower than 5.5 bar". A research ISX15 at part load: A:F 26.2 (lambda about 1.8), turbine in 686 K. No maker's T3 limit or full-load lambda for a truck engine found.
**Boost.** Forums only: a DD15 at 505 hp "pushing 32 psi"; a C15 "usually tops out at around 27 pounds", 38 psi at 1500 rpm on a grade. Not used for a row.
**Rows (report.py, the tractor):** BSFC at full load at 1200 rpm 175 to 205 g/kWh (cited); lambda there 1.25 to 2.5 (cited, the constraint; the top judgement); before the turbine at 1800 rpm 600 to 950 K (cited, the constraint; the floor judgement). Read on e43e228b with the fuel curve: 205.4 g/kWh, lambda 1.70, 805 K (2728 N m at 1200; 437 kW at 1800).

## 18. A heavy diesel's turbo matching, and its exhaust-to-intake pressure ratio (the lead's asks, 25 Nov 06:22 and 06:27 UTC)

**The practice, in the sources' words.** "the turbocharger compressor must be matched to the engine to avoid the surge area at torque peak and to stay in as high an efficiency area as possible at rated speed and load" (Woollenweber, US 5,025,629, a 350 BHP truck diesel rated at 2100 rpm: its rated point "at only 94% of the maximum speed" of the compressor; patent). "an engine full-load curve is to be completely within the usable compressor characteristic map; minimum clearances with respect to the characteristic map limits, as required by the vehicle manufacturer, are to be maintained; maximum compressor efficiencies are to be available at the rated load and in a range of a low-end apex torque" (Vitesco, US 10,900,497 B2; patent, passenger-car framing). "A good rule of thumb is to operate 10-percent or more to the right of the surge line" and "a turbocharger is sized to give sufficient surge margin, while also keeping the operational points on the compressor map at areas above 65-percent" (BorgWarner's application engineers, enginelabs.com: a maker's, in flow, the engine point not named, not heavy-duty specific). Garrett (Turbo Tech 103, gasoline examples): choke "where the efficiency drops below 58%"; below a speed "the boost pressure will be lower ... to keep the compressor out of surge". Fixes named: a ported shroud ("moves the surge line to the left", BorgWarner; "up to 15% additional surge margin" above a ratio of 3, Holset US 6,540,480 B2), a boost target falling at low speed (Garrett), a wider map (Vitesco), a variable compressor (Woollenweber). **Not found:** a stated surge margin at a heavy diesel's torque peak (Watson & Janota, Hiereth & Prenninger, Holset or Cummins Turbo Technologies notes not reachable); the VGT's role in it.
**p3/p2.** A Cummins 15 L SuperTruck II baseline at a cruise point (1130 rpm, 1437 N m, 170 kW; OSTI 1474075, Table 6, test data): turbine in 111.4 and intake manifold 101.6 kPa (gauge, read so from the compressor inlet's -0.74), compressor ratio 2.11, VGT 42.5 % closed, EGR 24.4 %: **p3/p2 about 1.05** (derived here, not stated). PACCAR's SuperTruck II (two-stage, low-pressure EGR): "pumping power is shown negative, meaning intake is at higher pressure than exhaust" (OSTI 2372616). **At peak torque (an estimate, a patent's):** Roberts and Stanglmaier, US 7,721,541 B2, on a 12 L heavy diesel at peak torque, 1200 rpm, 3.0 atm boost: "For a conventional, non-EGR engine, the positive cylinder-head pressure gradient is approximately 20 to 30 Kpa" (the intake above the exhaust); with high-pressure-loop EGR "the adverse engine cylinder-head pressure gradient necessary to produce reliable and controllable EGR flow is approximately 10 to 20 kPa" (the exhaust above), "a total pressure step of 40-60 Kpa" from the one to the other: **p3/p2 about 0.90 to 0.93 without EGR, 1.03 to 1.07 with it** (derived here at 3.0 atm; the patent's round figures, not test data); the EGR engine's agrees with the Cummins cruise point's 1.05. **The pumping's size (test data):** WVU for the ICCT (2014), a US 2010 heavy-duty diesel over the SET: 39.1 % of the fuel to brake work, **1.7 % to pumping** (some 4 % of the brake work), from motoring tests and a Volvo D13 mapped at full load against its back pressure (0.075 kg of fuel for each psig over the cycle); pumping on average 29 % of friction and pumping together. **Not found:** p3/p2 measured at peak torque or rated power on a 13-15 L engine; a stated surge margin at a heavy diesel's torque peak (the searches of 25 Nov 08:40 found the rule, "avoid the surge area at torque peak", and no number).

## 19. A turbo rotor's bearing loss and its run-down after shutdown (the lead's ask for N2, 25 Nov 07:04 UTC)

**Run-down, measured.** Gritsenko et al., Tribology in Industry 45(3), 2023 (tribology.rs, a paper; a K27-145 truck and tractor turbo, its wheel's size not stated; a ZMZ-4062 bench, oil from its own station at 50 / 70 / 90 C and 0.1 / 0.25 / 0.4 MPa): "at n = 25,000 rpm, rundown time was (10.3-14.8) s"; "at n = 50,000 rpm ... (28.1-34.4) s"; "at n = 75,000 rpm ... (38.6-47.1) s"; longer with hotter oil, shorter with higher oil pressure. A KAMAZ truck turbo (Orlov et al., cited there): "The rundown time was 18.6 s after 72,000 km", 22.8 s on an autonomous lubrication system (start speed not given). A ball-bearing turbo against a floating-ring one, otherwise alike (Brouwer, Purdue thesis / ASME 10.1115/1.4024780, rig driven by a 14 L diesel's compressor): "significantly lower frictional losses ... with an average increase in run-down time of 14.1%". Car turbos: forum timings only (5 to 15 s from idle).
**Bearing loss.** Serrano et al. (Energy 55, 2013; three car turbos, compressor 49 / 46 / 40 mm, a fixed and two floating-ring journals; oil flow times its rise): loss plotted to some 450 W over 30,000 to 190,000 rpm, no values in the text; "A linear correlation between the journal bearing geometry and parameter k jb and the area of the compressor wheel was found". A Korean measurement (abstract): loss rising as the speed to the 1.6, falling with hotter oil, rising with oil pressure. Windage theory (Brno 2022): as w^3 d^4. **Not found:** any maker's bearing loss in W; any 80 to 100 mm heavy-duty turbo's loss at a stated speed; a measured rotor inertia (an aviation turbo's 3.48e-4 kg m2, a search snippet).
So the class's measured run-down from 50,000 to 75,000 rpm is some 28 to 47 s, against the model's truck at 79 s and its small turbo at 23 s.

## 20. The diesel archetypes' peak-torque speeds, from their makers (the lead's ask for N2's matching rule, 25 Nov 08:58 UTC)

For the matching speed (a governed diesel's compressor sized with its torque-peak point some 10 % right of the surge line):
the lowest speed of the plateau is the one nearest surge (the least air at the most boost), so each is given as a range
with its start.
- **The 15 L tractor (the X15 kind), and D2's truck card** (the same 137 x 169 mm six): Cummins X15 Efficiency Series,
  Bulletin 5410582 (mart.cummins.com): **"peak torque starting at 950 rpm"**, flat to about 1300-1400 (read off its
  charts; section 15). The ISX15 560 / 1850 (Bulletin 4971488) "from 1150 rpm to 1500 rpm"; Detroit's DD15 "@ 975" or
  "@ 1100" by sheet (section 15). Cited. The tractor's own fuel curve plateaus from 1000 to 1400 rpm.
- **The 6.7 pickup** (the Cummins 6.7 in the Ram heavy duty): FCA's 2019 Ram 2500/3500 Heavy Duty specifications
  (chryslermedia, the maker's sheet): standard output **850 lb-ft @ 1,700 RPM**, 370 hp @ 2,800; high output **1000 lb-ft
  @ 1,800 RPM**, 400 hp @ 2,800 (the sheet prints 1,084 N m for both: a misprint, 1000 lb-ft is 1356 N m). A single
  speed, not a range. Cited. (Cummins' own release, 14 Jan 2019, gives the ratings without their speeds; the later
  1,075 lb-ft rating's speed not found.)
- **The 2.0 common rail** (the VW 2.0 TDI kind): Volkswagen's Golf Estate press presentation (volkswagen-newsroom.com,
  the Mk7): the 2.0 TDI 110 kW, **"maximum torque of 320 Nm is available from 1,750 rpm (and up to 3,000 rpm)"**,
  maximum power 3,500 to 4,000 rpm. Cited. (A later 340 N m EA288 evo is listed by spec sites at 1,600 to 3,000 rpm: not a
  maker's sheet.)
- **Not found:** a second source for p3/p2 at peak torque (section 18's patent's estimate the only one); Gundmalm's KTH
  thesis on divided exhaust periods (Scania, measured) unreachable on 25 Nov (the server down).

## 21. Compressor maps of the tractor's class: their width, and the speed margin kept at rated (the lead's ask for N2, 25 Nov 13:39 UTC)

**Read by me, from the makers' catalogue sheets' maps** (rendered and read off the gridlines: about 1 lb/min and 0.05 in ratio;
the edges taken as the ends of the drawn speed lines: the right end is where the maker stops drawing, efficiency some 0.54 to
0.64, not a measured choke):
- **BorgWarner S400SX-E, part 14009097006** (borgwarnerboosted.com catalogue p. 50, `s400sx-e.pdf`): compressor wheel 72.00 mm
  inducer, 96.00 mm outer; speed lines labelled in **corrected tip speed, 230 to 607 m/s**. Surge (the lines' left ends) and the
  right ends, lb/min: **PR 2.0: 24 and 95, width 4.0; PR 2.6: 35 and 106, width 3.0; PR 3.0: 43 and 110, width 2.6**. The top line
  (607 m/s, some 120,800 rpm on 96 mm) peaks near PR 5.0; at 561 m/s, 4.6; at 521, 4.1. Peak efficiency 0.80 near PR 2.1, 55
  lb/min. (Its larger sibling, 76 / 100 mm, 14009097013/14, on the same page: top line 605 m/s.)
- **BorgWarner S300SX3, parts 177281 / 177275** (catalogue p. 43, `s300sx3.pdf`): wheel 66.11 mm inducer, 91.44 mm outer; speed
  lines in rpm, 58,496 to 117,199. Surge and right ends, lb/min: **PR 2.0: 19 and 66, width 3.4; PR 2.6: 27 and 74, width 2.7; PR
  3.0: 32 and 76, width 2.35**. The top line (117,199 rpm: **561 m/s** at 91.44 mm) peaks near PR 4.7. Peak efficiency 0.76.
- Both are the makers' aftermarket wheels of the S300 / S400 frames (a heavy-duty frame family; rated by the maker for 320-800 and
  500-1200 hp), not an OE truck wheel's map. **Not found as a map with numbers:** Holset HX40 / HX55 / HE500 (Holset's sheets
  show maps without numbers, per forum reports; their numbered maps circulate as forum copies, not read), Garrett GT/GTX 40-45
  (not read this time).

**The speed margin at rated, at sea level (the altitude margin): not found as a figure in a maker's guide, Watson and Janota,
Heywood or an SAE paper read by me.** What there is: Woollenweber, US 5,025,629, the rated point "at only 94% of the maximum
speed" (one patent's example, section 18); studies of altitude matching that treat turbo speed as a limit and report where it
is reached (a TB28 over speed at 3500 m and 2600 rpm; an HP60 at 4000 m and 2800: IEEE 5701259, abstract only) without a
sea-level margin; and a forum's rule of thumb that turbo speed rises some **3 % per 1000 ft** (unverified). If 3 % per 1000 ft
held, a 6 % margin (Woollenweber's) would cover some 2000 ft (600 m); a truck sold for mountain passes (some 2500 m) would want
nearer 20 % (judgement, from the forum's rate).

**Continued: each speed line of the same two maps** (the lead's ask for N2's table, 25 Nov 13:56 UTC). PRINTED: what the sheet
prints as text. READ: read by me off the drawing, the maps rendered at 600 dpi (S400) and 300 dpi (S300) and read against the
gridlines: flow to about **±0.7 lb/min (S400) and ±1 lb/min (S300)**, pressure ratio to about **±0.03**. A line's surge end is
its drawn left end; its right end is where the maker stops drawing it (not a stated choke). Tip speed U = pi D N.

*Wheels and conditions (PRINTED):* S400SX-E 14009097006: "Comp. Wheel Inducer Dia. (mm) 72.00", "Comp. Wheel Outer Dia. (mm)
96.00" (trim (72/96)^2 = 56.3, computed); S300SX3 177281/177275: inducer 66.11, outer 91.44 mm (trim 52.3, computed). Flow axis
"Compressor Flow (lb/min)" on both; the S400's speed lines are labelled "uredC" in m/s (a corrected tip speed), its efficiency
"ηisC". **Not printed on either sheet: the flow's correction (corrected or actual), its reference temperature and pressure, and
the exducer as such** (the outer diameter is taken as the exducer). 

*S400SX-E (72 / 96 mm)*: speed PRINTED (uredC, m/s); rpm computed on 96.00 mm; the rest READ.

| uredC m/s | rpm | peak PR, at flow (lb/min) | surge end: flow, PR | right end: flow, PR |
|---|---|---|---|---|
| 230 | 45,760 | 1.41 at 15-18 | 12.0, 1.40 | 45.4, 1.22 |
| 310 | 61,670 | 1.82 at 21 | 21.2, 1.82 | 66.9, 1.42 |
| 370 | 73,610 | 2.25 at 29 (2.22 to 46) | 28.6, 2.25 | 81.2, 1.63 |
| 421 | 83,760 | 2.72 at 41-53 (flat) | 40.8, 2.72 | 90.5, 1.85 |
| 461 | 91,710 | 3.21 at 47 (3.20 at 63) | 47.1, 3.21 | 96.6, 2.05 |
| 491 | 97,680 | 3.62 at 62-68 | 62.2, 3.62 | 100.4, 2.22 |
| 521 | 103,650 | 4.10 at 65 | 64.8, 4.10 | 103.2, 2.40 |
| 561 | 111,610 | 4.63 at 72 | 71.6, 4.63 | 106.7, 2.69 |
| 607 | 120,760 | 4.99 at 78-79 (at the map's top edge) | 78.5, 4.99 | 110.3, 3.08 |

Efficiency (READ, ±0.02 on a contour's place): the island **ηisC = 0.8 (PRINTED)** at some **55 to 62 lb/min, PR 2.05 to 2.30**
(its centre about 58, 2.15). Along the top line (607) the highest contour crossed is 0.64 (PRINTED labels: it enters the 0.64
contour near 80 lb/min and leaves it near 107); along 561, 0.68 (the label sits on it near 73 lb/min), with 0.66 at both its
ends. The lines' right ends lie among the contours labelled at the map's right edge, 0.64 down to 0.54 (PRINTED), the lower
lines' ends nearer 0.6 to 0.62: **not assignable line by line by eye**.

*S300SX3 (66.11 / 91.44 mm)*: speed PRINTED (rpm); tip speed computed on 91.44 mm; the rest READ.

| rpm | U m/s | peak PR, at flow (lb/min) | surge end: flow, PR | right end: flow, PR |
|---|---|---|---|---|
| 58,496 | 280 | 1.68 at 15-21 | 14.9, 1.68 | 43.6, 1.39 |
| 77,362 | 370 | 2.34 at 26-34 | 25.7, 2.34 | 60.7, 1.78 |
| 92,449 | 443 | 3.14 at 35 | 35.3, 3.14 | 71.5, 2.25 |
| 105,437 | 505 | 3.94 at 46 (3.91 to 61) | 46.2, 3.94 | 75.7, 2.84 |
| 117,199 | 561 | 4.71 at 55 (4.69 to 66) | 55.1, 4.71 | 77.8, 3.64 |

Efficiency (READ): the highest contour **0.76 (PRINTED)**, its island near **45 to 52 lb/min, PR 2.4 to 2.8** (label at about
50, 2.77); 0.74 next (label near 59, 3.3). Along 117,199 the highest contour crossed is 0.66 to 0.68 (labels 0.66 near 58 lb/min
and 0.68 near 72, PR 4.4); along 105,437, 0.72 (its label on the line near 63 lb/min). The right ends lie between the contours
labelled 0.66 and 0.62 at the lower right: not assigned line by line.

*For N2's model (computed here from the READ peaks, not a source's):* the peak ratio against corrected tip speed falls on one
curve for both wheels within the reading's error: at U = 370 m/s, 2.25 (S400) and 2.34 (S300); at 561 m/s, 4.63 and 4.71.

## 22. The 2.0 diesel's and the turbo four's production turbos (the lead's ask for N2, 25 Nov 14:40 UTC)

- **The 2.0 common-rail diesel.** The build stands for the VW 2.0 TDI common rail (81 x 95.5 mm, 1968 cc; its notes: "103 to
  125 kW at 3500 to 4000 rpm, 320 to 380 N m from 1750"). **Its torque rows:** no row at 1750 rpm; "Peak torque" 320 to 380 N m
  and "Speed at peak torque" 1700 to 3000 rpm (`content/validation/archetypes/diesel-20-common-rail.json`); the published
  figure at 1750 is the plateau's start: VW's Mk7 Golf Estate presentation, "320 Nm is available from 1,750 rpm (and up to
  3,000 rpm)" for the 110 kW 2.0 TDI (section 20). **Its turbo:** sellers list a BorgWarner **BV43** as the OE turbo of the
  US common-rail 2.0 TDI (engine codes CBEA, CJAA: turbochargersdirect.com listings); **its compressor wheel's diameters were
  not found** on a maker's sheet or a parts catalogue (the listings give none). Garrett's GT1749V / GTA1749V kits are for the
  earlier pump-injection engines (BEW, BKD), not this one. Not found: the BV43's wheel and its map.
- **The turbo four (first.json).** Its `is20` wheels, compressor **42.5 mm inducer / 54.0 mm exducer**, turbine about 50 mm,
  came from table L's application of the EA888 sources file (`docs/measurements/validation/2026-11-24-2314-ea888-gen3-sources.md`,
  "The turbo's hardware"): mygolfmk7.com's table and a blog, a vwvortex and a NAM forum post (snippets); an eBay seller's
  41.8 / 53.96. **42.5 is the inducer.** No maker's figure. The turbo: **IHI IS20, 06K145874M** for the Golf GTI 2013-19
  (Garrett's PowerMax 898199 datasheet's OE table, read 25 Nov). **No published IS20 map found** (Garrett's PowerMax sheet
  maps its own replacement, not the IS20; IHI publishes none found).
