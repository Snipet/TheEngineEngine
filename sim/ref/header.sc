// header.sc: the panel's title bar. At the left the mark and the name; then the engine as it is set (its layout, its
// fuel, its air and how it is played); its state, a light and a word (off, cranking, running, stopping, the limiter
// and the fuel cut); at the right the output's peak level on a bar of lights, -48 to +6 dBFS.
// Inputs, in panel.gui's order: in0 engine, in1 fuel, in2 aspiration, in3 drive (dials: 0..1, their turn),
// in4 shared->showLamps (packed, engine.fm), in5 out (the output's peak, 0..1).
#include "funk/text.sh"
#include "funk/shapes.sh"

const vec3 ORANGE = vec3(1.0, 0.54, 0.12);
const vec3 INK = vec3(0.94, 0.95, 0.96);
const vec3 DIM = vec3(0.47, 0.50, 0.55);

// A rectangle of corners rounded by r, centred on c, half its size h: its coverage, smoothed over a pixel.
float roundRect(vec2 p, vec2 c, vec2 h, float r)
{
    vec2 q = abs(p - c) - h + vec2(r, r);
    float d = length(max(q, vec2(0.0, 0.0))) + min(max(q.x, q.y), 0.0) - r;
    return clamp(0.5 - d / u_px, 0.0, 1.0);
}

// The engine dial's layout i, by name.
float layoutName(vec2 p, vec2 at, float size, float i)
{
    if (i < 0.5) return text(p, at, size, 0.0, "SINGLE");
    if (i < 1.5) return text(p, at, size, 0.0, "TWIN 360");
    if (i < 2.5) return text(p, at, size, 0.0, "TWIN 270");
    if (i < 3.5) return text(p, at, size, 0.0, "V-TWIN 45");
    if (i < 4.5) return text(p, at, size, 0.0, "INLINE 3");
    if (i < 5.5) return text(p, at, size, 0.0, "INLINE 4");
    if (i < 6.5) return text(p, at, size, 0.0, "FLAT 4");
    if (i < 7.5) return text(p, at, size, 0.0, "X-PLANE I4");
    if (i < 8.5) return text(p, at, size, 0.0, "INLINE 5");
    if (i < 9.5) return text(p, at, size, 0.0, "V6");
    if (i < 10.5) return text(p, at, size, 0.0, "INLINE 6");
    if (i < 11.5) return text(p, at, size, 0.0, "FLAT 6");
    if (i < 12.5) return text(p, at, size, 0.0, "V8 CROSS");
    if (i < 13.5) return text(p, at, size, 0.0, "V8 FLAT");
    if (i < 14.5) return text(p, at, size, 0.0, "V10");
    if (i < 15.5) return text(p, at, size, 0.0, "V12");
    return text(p, at, size, 0.0, "ROTARY");
}

float fuelName(vec2 p, vec2 at, float size, float i)
{
    if (i < 0.5) return text(p, at, size, 0.0, "GASOLINE");
    return text(p, at, size, 0.0, "DIESEL");
}

float airName(vec2 p, vec2 at, float size, float i)
{
    if (i < 0.5) return text(p, at, size, 0.0, "NATURAL");
    if (i < 1.5) return text(p, at, size, 0.0, "TURBO");
    if (i < 2.5) return text(p, at, size, 0.0, "TWIN TURBO");
    if (i < 3.5) return text(p, at, size, 0.0, "QUAD TURBO");
    return text(p, at, size, 0.0, "S/CHARGED");
}

float driveName(vec2 p, vec2 at, float size, float i)
{
    if (i < 0.5) return text(p, at, size, 0.0, "KEYS");
    if (i < 1.5) return text(p, at, size, 0.0, "CAR");
    if (i < 2.5) return text(p, at, size, 0.0, "TRUCK");
    return text(p, at, size, 0.0, "PITCH");
}

// The state's word: 0 off, 1 cranking, 2 running, 3 stopping, 4 limiter, 5 fuel cut.
float stateName(vec2 p, vec2 at, float size, float s)
{
    if (s < 0.5) return text(p, at, size, 0.0, "OFF");
    if (s < 1.5) return text(p, at, size, 0.0, "CRANKING");
    if (s < 2.5) return text(p, at, size, 0.0, "RUNNING");
    if (s < 3.5) return text(p, at, size, 0.0, "STOPPING");
    if (s < 4.5) return text(p, at, size, 0.0, "LIMITER");
    return text(p, at, size, 0.0, "FUEL CUT");
}

void main()
{
    vec2 p = v_px;
    float W = u_size.x;
    float H = u_size.y;
    float my = 0.5 * H;

    // The ground: brushed dark metal, a light along the top edge, an orange line along the foot that fades out.
    vec3 col = mix(vec3(0.125, 0.13, 0.142), vec3(0.068, 0.072, 0.082), v_uv.y);
    col += 0.012 * sin(p.y * 2.7 + 0.35 * sin(p.x * 0.05));
    col += vec3(0.05, 0.05, 0.055) * (1.0 - smoothstep(0.0, 2.0, p.y));
    float foot = (1.0 - smoothstep(0.0, 1.6, H - 1.0 - p.y)) * (1.0 - smoothstep(0.15 * W, 0.75 * W, p.x));
    col = mix(col, ORANGE, 0.85 * foot);

    // The mark: a piston over its rod and crank.
    vec2 m = vec2(22.0, my);
    float mark = roundRect(p, m + vec2(0.0, -9.0), vec2(7.5, 5.0), 1.5);
    mark = max(mark, segment(p, m + vec2(0.0, -5.0), m + vec2(2.5, 7.0), 2.4));
    mark = max(mark, ring(p, m + vec2(0.0, 9.0), 5.0, 2.0));
    col = mix(col, ORANGE, mark);
    col += ORANGE * 0.10 * exp(-length(p - m) / 14.0);

    // The name.
    col = mix(col, INK, text(p, vec2(40.0, my - 5.0), 18.0, 0.0, "ENGINE"));
    col = mix(col, DIM, text(p, vec2(41.0, my + 11.0), 8.0, 0.0, "PHYSICAL MODEL"));
    col = mix(col, vec3(0.24, 0.25, 0.28), segment(p, vec2(172.0, 9.0), vec2(172.0, H - 9.0), 1.0));

    // The engine as it is set: four fields, a caption over a value.
    float x0 = 188.0;
    float pitch = clamp((W - 250.0 - x0) / 4.0, 72.0, 112.0);
    float cy = my - 8.0;
    float vy = my + 7.0;
    float x = x0;
    col = mix(col, DIM, text(p, vec2(x, cy), 8.0, 0.0, "LAYOUT"));
    col = mix(col, INK, layoutName(p, vec2(x, vy), 12.0, floor(in0 * 16.0 + 0.5)));
    x += pitch;
    col = mix(col, vec3(0.22, 0.23, 0.26), segment(p, vec2(x - 12.0, 11.0), vec2(x - 12.0, H - 11.0), 1.0));
    col = mix(col, DIM, text(p, vec2(x, cy), 8.0, 0.0, "FUEL"));
    col = mix(col, INK, fuelName(p, vec2(x, vy), 12.0, floor(in1 + 0.5)));
    x += pitch;
    col = mix(col, vec3(0.22, 0.23, 0.26), segment(p, vec2(x - 12.0, 11.0), vec2(x - 12.0, H - 11.0), 1.0));
    col = mix(col, DIM, text(p, vec2(x, cy), 8.0, 0.0, "AIR"));
    col = mix(col, INK, airName(p, vec2(x, vy), 12.0, floor(in2 * 4.0 + 0.5)));
    x += pitch;
    col = mix(col, vec3(0.22, 0.23, 0.26), segment(p, vec2(x - 12.0, 11.0), vec2(x - 12.0, H - 11.0), 1.0));
    col = mix(col, DIM, text(p, vec2(x, cy), 8.0, 0.0, "PLAYED BY"));
    col = mix(col, INK, driveName(p, vec2(x, vy), 12.0, floor(in3 * 3.0 + 0.5)));

    // The state, from the dashboard's lamps: a light and its word.
    float lamps = mod(in4, 32.0);
    float st = mod(lamps, 4.0);
    float lim = mod(floor(lamps / 4.0), 2.0);
    float cut = mod(floor(lamps / 8.0), 2.0);
    float running = step(1.5, st) * (1.0 - step(2.5, st));
    float s = st;
    s = mix(s, 5.0, cut * running);
    s = mix(s, 4.0, lim * running);
    float blink = step(0.0, sin(u_time * 18.0));
    vec3 lampCol = vec3(0.30, 0.31, 0.33);
    lampCol = mix(lampCol, vec3(1.0, 0.70, 0.15) * (0.4 + 0.6 * blink), step(0.5, st) * (1.0 - step(1.5, st)));
    lampCol = mix(lampCol, vec3(0.25, 0.95, 0.45), running);
    lampCol = mix(lampCol, vec3(1.0, 0.62, 0.10) * 0.8, step(2.5, st));
    lampCol = mix(lampCol, vec3(1.0, 0.62, 0.10), cut * running);
    lampCol = mix(lampCol, vec3(1.0, 0.18, 0.12) * (0.45 + 0.55 * blink), lim * running);
    float lit = step(0.5, st);
    float xs = W - 236.0;
    col = mix(col, vec3(0.22, 0.23, 0.26), segment(p, vec2(xs - 12.0, 11.0), vec2(xs - 12.0, H - 11.0), 1.0));
    col = mix(col, DIM, text(p, vec2(xs, cy), 8.0, 0.0, "STATE"));
    vec2 lc = vec2(xs + 4.0, vy);
    col += lampCol * lit * 0.45 * exp(-length(p - lc) / 6.0);
    col = mix(col, vec3(0.05, 0.05, 0.06), circle(p, lc, 5.0));
    col = mix(col, lampCol, circle(p, lc, 3.8));
    col = mix(col, mix(INK, lampCol, 0.35 * lit), stateName(p, vec2(xs + 14.0, vy), 11.0, s));

    // The output: its peak in dBFS on 24 lights, green to -12, amber to -3, red above; the number over them.
    float mx0 = W - 132.0;
    float mx1 = W - 16.0;
    float level = max(in5, 0.0);
    float db = 20.0 * log(max(level, 0.00001)) / log(10.0);
    float norm = clamp((db + 48.0) / 54.0, 0.0, 1.0);
    col = mix(col, vec3(0.22, 0.23, 0.26), segment(p, vec2(mx0 - 12.0, 11.0), vec2(mx0 - 12.0, H - 11.0), 1.0));
    col = mix(col, DIM, text(p, vec2(mx0, cy), 8.0, 0.0, "OUTPUT"));
    col = mix(col, mix(DIM, INK, step(-47.9, db)), number(p, vec2(mx1, cy), 9.0, 1.0, max(db, -48.0), 1.0));
    float n = 24.0;
    float sw = (mx1 - mx0) / n;
    float u = (p.x - mx0) / sw;
    float i = floor(u);
    float f = u - i;
    float inBar = step(0.0, u) * step(u, n) * step(0.18, f) * step(f, 0.82);
    inBar *= step(vy - 4.0, p.y) * step(p.y, vy + 4.0);
    float at = (i + 0.5) / n;
    vec3 segCol = vec3(0.20, 0.90, 0.42);
    segCol = mix(segCol, vec3(1.0, 0.70, 0.12), step(36.0 / 54.0, at));
    segCol = mix(segCol, vec3(1.0, 0.20, 0.12), step(45.0 / 54.0, at));
    float on = step(at, norm) * step(0.001, norm);
    vec3 led = mix(segCol * 0.16 + vec3(0.03, 0.03, 0.035), segCol, on);
    col = mix(col, led, inBar);
    float glowX = clamp(p.x, mx0, mx0 + norm * (mx1 - mx0));
    col += vec3(1.0, 0.6, 0.2) * 0.10 * step(0.001, norm) * exp(-length(p - vec2(glowX, vy)) / 7.0);

    gl_FragColor = vec4(col, roundRect(p, 0.5 * u_size, 0.5 * u_size, 8.0));
}
