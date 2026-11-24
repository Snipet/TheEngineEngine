# The back pressure the cylinder sees under boost (first question)

Measured on 6af5bec (before card G's exhaust network is wired; with D's e9a9748: Woschni walls and Chen-Flynn
friction under fuel.model 1), `content/builds/archetypes/turbo-four-20-gdi.json` (every model switch on; target 120
kPa, spring 60 kPa, twin scroll, matched sizes):

    tee-calib-boost --build content/builds/archetypes/turbo-four-20-gdi.json --rpm 2000,3500,5000,6500 --settle 5 --measure 1

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2000 | 383 | 24.3 | 24.9 | -0.82 | 1.31 | 1.98 | 126 | 221 | 326 | 216 | 134 | 220 | 0.61 | 0.09 | 136 | 0.61 | 0.0 | 0.82 | 2.1 | 20.4 | 13.1 | 930 |
| 3500 | 387 | 24.5 | 25.7 | -0.52 | 1.62 | 1.92 | 125 | 219 | 333 | 231 | 153 | 219 | 0.70 | 0.39 | 131 | 0.73 | 0.0 | 0.82 | 10.9 | 11.7 | 3.9 | 1048 |
| 5000 | 383 | 24.3 | 26.0 | -0.28 | 2.00 | 1.88 | 127 | 218 | 337 | 252 | 170 | 218 | 0.78 | 0.56 | 134 | 0.75 | 0.0 | 0.82 | 13.4 | 9.1 | 1.5 | 1184 |
| 6500 | 363 | 23.0 | 25.7 | 0.28 | 2.44 | 1.84 | 130 | 217 | 344 | 287 | 188 | 217 | 0.87 | 0.63 | 147 | 0.73 | 0.0 | 0.82 | 14.5 | 8.0 | 0.8 | 1259 |

Units: pressures kPa (charge pipe over the air, the rest absolute); "turbine in" is `tel.boost.turbineIn[0]`, the
stand-in housing's own pressure (Boost::exhaustStandIn), which no pipe sees; "exhaust port mean" is what cylinder 1's
exhaust valve opens onto; shaft in thousands of rpm; spark degrees before the top; half-burn degrees after.

**Reading.** The cylinder exhausts at 0.61 to 0.87 of its intake port's pressure, while the housing it should be
exhausting into stands at 0.98 to 1.32 of the plenum (216 to 287 kPa). A matched turbo at full load runs a turbine
inlet at or above the boost (1.0 to 1.5; Watson and Janota, *Turbocharging the Internal Combustion Engine*, ch. 7;
Heywood ch. 6.8). So the cylinder sees 80 to 100 kPa less back pressure than its turbine makes: its pumping loop gives
work (PMEP -0.82 to -0.28 bar where it should take some 0.5 to 1 bar) and it over-scavenges (VE on the manifold 1.01 to
1.04 at 2.2 bar; with a turbine inlet above the intake it would be some 0.85 to 0.95). Cause: with boost.model 1,
engine.fm passes the reference's turbine restriction as 0 (aspUse 0 to exh.pipes) and the stand-in hands no
pressure back to the pipes. Card G's housing seam ends it.

Also measured (not causes yet): knock retard 13 degrees at 2000 rpm, 1 to 4 above 3500; lambda 0.82 everywhere;
intercooler out 326 to 344 K at 288 K outside, no air through it (the dyno); BMEP 24.3 to 24.5 bar to 5000 rpm, 23.0 at
6500 (the class: 22 to 25 bar at peak torque, so torque is now in range on this commit; the 470 N m of the table at
463b18f was before e9a9748); power at 6500 rpm 247 kW against 155 to 230.
