// glow.sc: the light round something hot (a header, a turbine housing, a brake disc): a soft halo that breathes a
// little, strongest at the rect's middle and gone at its edges. Light only: nothing a person reads; take it away and no
// fact is lost (the part's own drawing carries its temperature in numbers and a tint).
// Inputs (u_params, set by ui/views/fx/Fx.h): in0 heat 0..1 (0 draws nothing), in1..in3 the colour (0..1 rgb, the
// palette's hot), in4 a phase that only the application's step(dt) moves (no clock is read here), in5 the halo's
// share of the rect taken by the part itself (0..0.9: the part's own shape is left clear in the middle).

void main()
{
    vec2 c = v_uv * 2.0 - vec2(1.0, 1.0);                     // -1..1 across the rect
    float r = length(c);
    float inner = clamp(in5, 0.0, 0.9);
    // A halo from the part's edge out to the rect's: brightest at the part, falling as a soft square law.
    float t = clamp((r - inner) / max(1.0 - inner, 0.05), 0.0, 1.0);
    float halo = (1.0 - t) * (1.0 - t);
    float breathe = 0.85 + 0.15 * sin(6.2831853 * in4);
    float a = clamp(in0, 0.0, 1.0) * halo * breathe * 0.55;
    gl_FragColor = vec4(clamp(vec3(in1, in2, in3), 0.0, 1.0), a);
}
