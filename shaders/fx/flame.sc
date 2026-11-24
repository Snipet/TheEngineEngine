// flame.sc: flame at a tail pipe on the overrun (a pop): a short tongue of fire leaving the pipe's end, along the
// rect's x (from x = 0 when in2 is 0, from the right when in2 is 1), yellow at its root, orange to red at its tips,
// flickering. Light only: the pop itself is in the telemetry and its words; this is the picture of it.
// Inputs: in0 strength 0..1 (0 draws nothing; it is the pop's size, eased down by the caller), in1 a phase moved only
// by step(dt), in2 0 = the pipe at the left, 1 = at the right.

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
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
    float s = clamp(in0, 0.0, 1.0);
    float x = mix(v_uv.x, 1.0 - v_uv.x, step(0.5, in2));     // 0 at the pipe, 1 at the tongue's farthest
    float y = v_uv.y * 2.0 - 1.0;
    float len = 0.35 + 0.65 * s;
    float n = noise(vec2(x * 6.0 - in1 * 9.0, y * 3.0 + in1 * 2.0));
    // The tongue: wide at the pipe, narrowing to its tip, its edge broken by the noise.
    float width = (1.0 - x / len) * (0.75 + 0.35 * n);
    float body = clamp((width - abs(y)) * 6.0, 0.0, 1.0) * step(x, len);
    vec3 root = vec3(1.0, 0.86, 0.45);
    vec3 tip = vec3(0.95, 0.28, 0.08);
    vec3 col = mix(root, tip, clamp(x / len + 0.3 * n, 0.0, 1.0));
    gl_FragColor = vec4(col, body * s * 0.9);
}
