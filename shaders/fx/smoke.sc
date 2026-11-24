// smoke.sc: tyre smoke (or a diesel's soot) drifting up and back from a wheel: soft billows that rise and thin out
// towards the rect's top. Light and atmosphere only: the wheel's slip is in its bar and its number.
// Inputs: in0 density 0..1 (0 draws nothing: the slip, eased by the caller), in1 a phase moved only by step(dt),
// in2..in4 the smoke's colour (0..1 rgb: the palette's dim ink, so it reads on either ground).

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(269.5, 183.3))) * 43758.5453);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

void main()
{
    float d = clamp(in0, 0.0, 1.0);
    vec2 p = vec2(v_uv.x * 3.0 + in1 * 1.5, v_uv.y * 2.0 + in1 * 2.5);
    float n = 0.55 * noise(p) + 0.3 * noise(p * 2.1 + vec2(3.1, 1.7)) + 0.15 * noise(p * 4.3);
    float rise = 1.0 - v_uv.y;                                  // thinner towards the top
    float shape = clamp(1.0 - abs(v_uv.x * 2.0 - 1.0), 0.0, 1.0) * clamp(1.0 - rise * 0.9, 0.0, 1.0);
    float a = d * clamp(n * 1.6 - 0.35, 0.0, 1.0) * shape * 0.6;
    gl_FragColor = vec4(clamp(vec3(in2, in3, in4), 0.0, 1.0), a);
}
