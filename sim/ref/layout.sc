// layout.sc: the engine from above. Its cylinders fire in their order (slowed, so the order can be followed: the real
// cycle goes by too fast to see), the valves open and shut (exhaust amber, intake blue), the pulses run down the headers
// into the collectors (their cones further on the longer the headers), the pipes (the muffler further on the longer the
// main pipe) and the mufflers and out of the tailpipes; the plenum and the throttle's plate; the
// turbos when there are some (one, two or four), spinning with their boost, or a supercharger on the plenum, its rotors
// turning with the crank; an X-pipe between two pipes when there is one; the firing order written out under it, the
// cylinder that fired last lit; the bores as wide as BORE/STROKE makes them. A rotary: its two rotors turning in their
// housings, the plugs flashing.
// in0 the engine dial (0 to 1: the layout 0 to 16), in1 a slowed copy of the crank's cycle (0 to 1), in2 the lamps (the
// state + 4 limiter + 8 fuel cut + 16 driven), in3 the throttle (0 to 1), in4 the forced induction (0 none; 1 a turbo,
// 2 twin turbos, 3 quad turbos, 4 a supercharger, plus the BOOST dial's turn under 1) and ten times the BORE/STROKE
// dial (1 to 100: 0.5 to 2; 0, before the sound starts: square), in5 the HEADERS, PIPE and X-PIPE
// dials (each 0 to 99, packed: headers * 10000 + pipe * 100 + X-pipe), in6 the boost (bar), in7 the output's level (0
// to 1)

const float G_F = 31140.0;
const float G_I = 29847.0;
const float G_R = 27565.0;
const float G_N = 27501.0;
const float G_G = 14699.0;
const float G_O = 11114.0;
const float G_D = 27502.0;
const float G_E = 31143.0;
const float G_MINUS = 448.0;

float digitGlyph(float d)
{
    if (d < 0.5)
        return 31599.0;
    if (d < 1.5)
        return 11415.0;
    if (d < 2.5)
        return 25255.0;
    if (d < 3.5)
        return 25230.0;
    if (d < 4.5)
        return 23497.0;
    if (d < 5.5)
        return 31118.0;
    if (d < 6.5)
        return 14762.0;
    if (d < 7.5)
        return 29330.0;
    if (d < 8.5)
        return 10922.0;
    return 10958.0;
}

// the layouts (layouts.fm): cylinders, how they are drawn (0 inline, 1 vee, 2 flat, 3 rotary), which bank each is drawn in
// (bit i: cylinder i), the exhaust (0 one pipe, 1 two collectors into one pipe, 2 two pipes), the firing angles
vec4 layoutOf(float k)
{
    if (k < 0.5) return vec4(1.0, 0.0, 0.0, 0.0);
    if (k < 1.5) return vec4(2.0, 0.0, 0.0, 0.0);
    if (k < 2.5) return vec4(2.0, 0.0, 0.0, 0.0);
    if (k < 3.5) return vec4(2.0, 1.0, 2.0, 0.0);
    if (k < 4.5) return vec4(3.0, 0.0, 0.0, 0.0);
    if (k < 5.5) return vec4(4.0, 0.0, 0.0, 0.0);
    if (k < 6.5) return vec4(4.0, 2.0, 10.0, 1.0);
    if (k < 7.5) return vec4(4.0, 0.0, 0.0, 0.0);
    if (k < 8.5) return vec4(5.0, 0.0, 0.0, 0.0);
    if (k < 9.5) return vec4(6.0, 1.0, 42.0, 2.0);
    if (k < 10.5) return vec4(6.0, 0.0, 0.0, 1.0);
    if (k < 11.5) return vec4(6.0, 2.0, 56.0, 2.0);
    if (k < 12.5) return vec4(8.0, 1.0, 170.0, 2.0);
    if (k < 13.5) return vec4(8.0, 1.0, 240.0, 2.0);
    if (k < 14.5) return vec4(10.0, 1.0, 992.0, 2.0);
    if (k < 15.5) return vec4(12.0, 1.0, 4032.0, 2.0);
    if (k < 16.5) return vec4(6.0, 3.0, 0.0, 0.0);
    return vec4(1.0, 0.0, 0.0, 0.0);
}

vec4 fireA(float k)
{
    if (k < 0.5) return vec4(0.0, 0.0, 0.0, 0.0);
    if (k < 1.5) return vec4(0.0, 360.0, 0.0, 0.0);
    if (k < 2.5) return vec4(0.0, 270.0, 0.0, 0.0);
    if (k < 3.5) return vec4(0.0, 315.0, 0.0, 0.0);
    if (k < 4.5) return vec4(0.0, 240.0, 480.0, 0.0);
    if (k < 5.5) return vec4(0.0, 540.0, 180.0, 360.0);
    if (k < 6.5) return vec4(0.0, 360.0, 180.0, 540.0);
    if (k < 7.5) return vec4(0.0, 450.0, 270.0, 540.0);
    if (k < 8.5) return vec4(0.0, 144.0, 576.0, 288.0);
    if (k < 9.5) return vec4(0.0, 120.0, 240.0, 360.0);
    if (k < 10.5) return vec4(0.0, 480.0, 240.0, 600.0);
    if (k < 11.5) return vec4(0.0, 240.0, 480.0, 360.0);
    if (k < 12.5) return vec4(0.0, 630.0, 270.0, 180.0);
    if (k < 13.5) return vec4(0.0, 540.0, 180.0, 360.0);
    if (k < 14.5) return vec4(0.0, 288.0, 432.0, 576.0);
    if (k < 15.5) return vec4(0.0, 480.0, 240.0, 600.0);
    if (k < 16.5) return vec4(0.0, 360.0, 720.0, 180.0);
    return vec4(0.0);
}

vec4 fireB(float k)
{
    if (k < 7.5) return vec4(0.0, 0.0, 0.0, 0.0);
    if (k < 8.5) return vec4(432.0, 0.0, 0.0, 0.0);
    if (k < 9.5) return vec4(480.0, 600.0, 0.0, 0.0);
    if (k < 10.5) return vec4(120.0, 360.0, 0.0, 0.0);
    if (k < 11.5) return vec4(600.0, 120.0, 0.0, 0.0);
    if (k < 12.5) return vec4(450.0, 360.0, 540.0, 90.0);
    if (k < 13.5) return vec4(90.0, 630.0, 270.0, 450.0);
    if (k < 14.5) return vec4(144.0, 72.0, 360.0, 504.0);
    if (k < 15.5) return vec4(120.0, 360.0, 60.0, 540.0);
    if (k < 16.5) return vec4(540.0, 900.0, 0.0, 0.0);
    return vec4(0.0);
}

vec4 fireC(float k)
{
    if (k < 13.5) return vec4(0.0, 0.0, 0.0, 0.0);
    if (k < 14.5) return vec4(648.0, 216.0, 0.0, 0.0);
    if (k < 15.5) return vec4(300.0, 660.0, 180.0, 420.0);
    return vec4(0.0);
}

float fireAt(float k, float i)
{
    vec4 v = mix(mix(fireC(k), fireB(k), step(i, 7.5)), fireA(k), step(i, 3.5));
    float j = i < 3.5 ? i : (i < 7.5 ? i - 4.0 : i - 8.0);
    return j < 0.5 ? v.x : (j < 1.5 ? v.y : (j < 2.5 ? v.z : v.w));
}

float bankOf(float mask, float i)
{
    return mod(floor((mask + 0.5) / exp2(i)), 2.0);
}

float seg(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

float glyph(vec2 p, vec2 o, float u, float bits)
{
    vec2 q = (p - o) / u;
    q.y = -q.y;
    if (q.x < 0.0 || q.x >= 3.0 || q.y < 0.0 || q.y >= 5.0)
        return 0.0;
    vec2 cell = floor(q);
    float idx = (4.0 - cell.y) * 3.0 + (2.0 - cell.x);
    float on = mod(floor((bits + 0.5) / exp2(idx)), 2.0);
    vec2 f = fract(q) - 0.5;
    return on * smoothstep(0.62, 0.38, max(abs(f.x), abs(f.y)));
}

// a rounded box's signed distance: centre c, half size h, corner r
float box(vec2 p, vec2 c, vec2 h, float r)
{
    vec2 q = abs(p - c) - h + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// an equilateral triangle's signed distance (its circumradius r), turned by a radians
float tri(vec2 p, float r, float a)
{
    float ca = cos(a);
    float sa = sin(a);
    p = vec2(ca * p.x + sa * p.y, -sa * p.x + ca * p.y);
    float k = 1.7320508;
    float h = r * 0.8660254;
    p.x = abs(p.x) - h;
    p.y = p.y + h / k;
    if (p.x + k * p.y > 0.0)
        p = vec2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0 * h, 0.0);
    return -length(p) * sign(p.y);
}

// a line drawn d from it, w wide, in colour c over col
vec3 ink(vec3 col, float d, float w, vec3 c)
{
    return mix(col, c, smoothstep(w, w * 0.4, d));
}

// a point at distance t along the path a-b-c-d
vec2 along(vec2 a, vec2 b, vec2 c, vec2 d, float t)
{
    float l1 = length(b - a);
    float l2 = length(c - b);
    float l3 = length(d - c);
    if (t < l1)
        return a + (b - a) * (t / max(l1, 0.0001));
    t -= l1;
    if (t < l2)
        return b + (c - b) * (t / max(l2, 0.0001));
    t -= l2;
    return c + (d - c) * clamp(t / max(l3, 0.0001), 0.0, 1.0);
}

// a turbo at c: its snail housing, the wheel turning
vec3 turbo(vec3 col, vec2 p, vec2 c, float r, float spin, float glow)
{
    vec2 q = p - c;
    float d = length(q);
    col = mix(col, vec3(0.07, 0.08, 0.10), smoothstep(r, r * 0.94, d));
    col = ink(col, abs(d - r), 0.006, vec3(0.55, 0.65, 0.78));
    col = ink(col, abs(d - r * 0.62), 0.004, vec3(0.40, 0.48, 0.58));
    float a = atan(q.y / (abs(q.x) + 0.00001));
    if (q.x < 0.0)
        a = 3.14159265 - a;
    float b = abs(fract((a + spin) / 6.2831853 * 7.0) - 0.5);
    float blade = smoothstep(0.08, 0.02, b) * step(d, r * 0.58) * step(r * 0.12, d);
    col = mix(col, vec3(0.75, 0.82, 0.90), blade * 0.8);
    col += vec3(1.0, 0.55, 0.15) * glow * 0.25 * exp(-d / r);
    return col;
}

// a supercharger at c: its case, and two rotors of three lobes turning against each other (ang: how far)
vec3 blower(vec3 col, vec2 p, vec2 c, float r, float ang, float glow)
{
    float dc = box(p, c, vec2(1.75 * r, 1.08 * r), 0.3 * r);
    col = mix(col, vec3(0.07, 0.08, 0.10), smoothstep(0.003, -0.003, dc));
    col = ink(col, abs(dc), 0.005, vec3(0.55, 0.65, 0.78));
    for (int s = 0; s < 2; ++s)
    {
        float sg = s == 0 ? 1.0 : -1.0;
        vec2 q = p - c + vec2(sg * 0.68 * r, 0.0);
        float a = atan(q.y / (abs(q.x) + 0.00001));
        if (q.x < 0.0)
            a = 3.14159265 - a;
        float d = length(q) - r * (0.62 + 0.32 * cos(3.0 * (a - sg * ang)));
        col = mix(col, vec3(0.34, 0.40, 0.48), smoothstep(0.003, -0.003, d));
        col = ink(col, abs(d), 0.004, vec3(0.80, 0.86, 0.94));
        col = mix(col, vec3(0.12, 0.14, 0.17), smoothstep(0.16 * r, 0.10 * r, length(q)));
    }
    col += vec3(1.0, 0.55, 0.15) * glow * 0.2 * exp(-length(p - c) / (2.0 * r));
    return col;
}

void main()
{
    vec2 p = (v_px - 0.5 * u_size) / u_size.y;
    p.y = -p.y;
    float aspect = u_size.x / u_size.y;
    // a narrow box: the drawing made smaller, to fit across it
    float zoom = min(1.0, aspect / 1.4);
    p /= zoom;
    aspect /= zoom;
    float kk = floor(clamp(in0, 0.0, 1.0) * 16.0 + 0.5);
    vec4 L = layoutOf(kk);
    float n = L.x;
    float fam = L.y;
    float mask = L.z;
    float topo = L.w;
    float cyc = fam > 2.5 ? 1080.0 : 720.0;
    float A = fract(max(in1, 0.0)) * cyc;
    float lamps = max(in2, 0.0);
    float st = mod(lamps, 4.0);
    float cut = mod(floor(lamps / 8.0), 2.0);
    float thr = clamp(in3, 0.0, 1.0);
    float air4 = max(in4, 0.0);
    float bq = floor(air4 / 10.0 + 0.00001);
    float air = air4 - 10.0 * bq;
    float kind = floor(air + 0.0001);
    float tb = clamp(air - kind, 0.0, 1.0);
    float bsr = bq < 0.5 ? 1.0 : 0.5 + 1.5 * clamp((bq - 1.0) / 99.0, 0.0, 1.0);
    float pk5 = max(in5, 0.0);
    float hq = floor(pk5 / 10000.0 + 0.00001);
    float pq = floor((pk5 - hq * 10000.0) / 100.0 + 0.00001);
    float hN = clamp(hq / 99.0, 0.0, 1.0);
    float pN = clamp(pq / 99.0, 0.0, 1.0);
    float xp = clamp((pk5 - hq * 10000.0 - pq * 100.0) / 99.0, 0.0, 1.0);
    float boost = max(in6, 0.0);
    float level = clamp(in7, 0.0, 1.0);
    float alive = st > 1.5 ? 1.0 : (st > 0.5 ? 0.5 : 0.0);
    float burning = st > 1.5 && st < 2.5 && cut < 0.5 ? 1.0 : (st > 0.5 && st < 1.5 ? 0.5 : 0.0);
    float load = 0.35 + 0.65 * thr;

    // the ground: a blueprint's
    vec3 col = vec3(0.030, 0.040, 0.055);
    vec2 gq = abs(fract(p * 16.0) - 0.5);
    col += vec3(0.020, 0.030, 0.042) * smoothstep(0.46, 0.5, max(gq.x, gq.y));
    vec3 steel = vec3(0.50, 0.60, 0.72);
    vec3 dimSteel = vec3(0.26, 0.32, 0.40);
    vec3 hot = vec3(1.0, 0.50, 0.14);
    vec3 cold = vec3(0.35, 0.78, 1.0);
    vec3 pipeCol = mix(dimSteel, vec3(0.70, 0.36, 0.14), 0.55 * alive * load * (0.4 + 0.6 * level));

    // where things go
    float W = aspect;
    float bx0 = -0.5 * W + 0.20;
    float bx1 = -0.5 * W + 0.56 * W;
    float exitX = 0.5 * W - 0.07;
    float yC = 0.06;

    // the banks' sizes
    float n0 = 0.0;
    for (int ii = 0; ii < 12; ++ii)
    {
        float i = float(ii);
        if (i < n - 0.5)
            n0 += 1.0 - bankOf(mask, i);
    }
    float n1 = n - n0;
    float rowN = fam > 2.5 ? 2.0 : max(n0, n1);
    float stag = fam > 0.5 && fam < 2.5 ? 0.38 : 0.0;
    float pitch = fam > 2.5 ? min(0.46, (bx1 - bx0) / 1.6) : min(0.19, (bx1 - bx0) / max(rowN + stag, 1.0));
    float rsz = pitch / 0.34;   // a rotary's housings, drawn to its pitch
    // (the bores as BORE/STROKE's cube root: a square engine's as they were)
    float rb = fam > 2.5 ? 0.0 : min(min(0.40 * pitch, 0.075) * pow(bsr, 0.3333333), 0.47 * pitch);
    float rowLen = (rowN - 1.0) * pitch + stag * pitch;
    float xs = 0.5 * (bx0 + bx1) - 0.5 * rowLen;
    float xe = xs + rowLen;
    float yOff = fam < 0.5 ? 0.0 : (fam < 1.5 ? 0.15 : 0.25);
    if (fam > 2.5)
        yOff = 0.0;
    // the collectors (top and bottom) and the plenum
    float ext = fam > 2.5 ? 0.13 * rsz : rb + 0.10;
    float yT = yC + yOff + ext;
    float yB = yC - yOff - ext;
    float yP = yC + (fam < 0.5 || fam > 2.5 ? ext : 0.0);
    bool two = fam > 0.5 && fam < 2.5;
    bool dual = two && topo > 1.5;
    // the collectors end (their cones) further on the longer the headers; the main pipe's length puts the muffler
    // nearer or further (after the turbos, when there are some)
    float xc = bx1 + 0.03 + 0.12 * hN;
    float xm = xc + 0.10;
    float turbos = kind > 0.5 && kind < 3.5 ? 1.0 : 0.0;
    float mw = 0.085;
    float mL0 = two && !dual ? xm + 0.04 + 0.04 * turbos * step(kind, 1.5) : xc + 0.05 + 0.15 * turbos;
    float mL1 = max(exitX - 0.035 - 2.0 * mw, mL0);
    float mx = mix(mL0, mL1, pN) + mw;
    float yOut = two && !dual ? yC : yB;

    // ---- the pipes: collectors, merge, mufflers, tailpipes ----
    float pw = 0.010;
    float dP = 1.0e3;
    // (the headers' run along to the collectors' cones: a bundle of pipes, drawn as two)
    float dH = 1.0e3;
    if (two)
    {
        dH = min(dH, seg(p, vec2(xs - 0.02, yT), vec2(xc - 0.04, yT)));
        dH = min(dH, seg(p, vec2(xs - 0.02, yB), vec2(xc - 0.04, yB)));
        if (dual)
        {
            dP = min(dP, seg(p, vec2(xc, yT), vec2(exitX, yT)));
            dP = min(dP, seg(p, vec2(xc, yB), vec2(exitX, yB)));
        }
        else
        {
            dP = min(dP, seg(p, vec2(xc, yT), vec2(xm, yC)));
            dP = min(dP, seg(p, vec2(xc, yB), vec2(xm, yC)));
            dP = min(dP, seg(p, vec2(xm, yC), vec2(exitX, yC)));
        }
    }
    else
    {
        dH = min(dH, seg(p, vec2(xs - 0.02, yB), vec2(xc - 0.04, yB)));
        dP = min(dP, seg(p, vec2(xc, yB), vec2(exitX, yB)));
    }
    col = ink(col, dP, pw, pipeCol);
    col = ink(col, abs(dH - 0.0065), 0.0042, pipeCol);
    // the collectors' cones, where the headers end
    {
        float cx = clamp(p.x, xc - 0.04, xc);
        float cw = pw * (2.3 - 1.3 * (cx - xc + 0.04) / 0.04);
        float dc = length(p - vec2(cx, two ? yT : yB));
        if (two)
            dc = min(dc, length(p - vec2(cx, yB)));
        col = ink(col, dc, cw, pipeCol * 1.15);
    }
    // the X-pipe, halfway along the main pipes
    if (dual && xp > 0.01)
    {
        float s0 = xc + 0.02 + 0.13 * turbos;
        float s1 = mx - mw - 0.02;
        float xw = clamp(s1 - s0, 0.03, 0.12);
        float x0 = 0.5 * (s0 + s1) - 0.5 * xw;
        float x1 = x0 + xw;
        float dx = min(seg(p, vec2(x0, yT), vec2(x1, yB)), seg(p, vec2(x0, yB), vec2(x1, yT)));
        col = mix(col, ink(col, dx, pw * 0.8, pipeCol), xp);
    }
    // the mufflers and the tips
    {
        float mh = 0.045;
        float dm = box(p, vec2(mx, yOut), vec2(mw, mh), 0.03);
        if (dual)
            dm = min(dm, box(p, vec2(mx, yT), vec2(mw, mh), 0.03));
        col = mix(col, vec3(0.06, 0.07, 0.09), smoothstep(0.003, -0.003, dm));
        col = ink(col, abs(dm), 0.005, steel * 0.8);
        float dt = length(p - vec2(exitX, yOut));
        if (dual)
            dt = min(dt, length(p - vec2(exitX, yT)));
        col = ink(col, abs(dt - 0.018), 0.005, vec3(0.80, 0.82, 0.86));
        col += hot * alive * load * 0.25 * exp(-dt / 0.03) * (0.4 + 0.6 * level);
    }
    // the turbos: one (where two banks' pipes meet, or on the one pipe; two pipes share it), two (one a bank, or a pair
    // on the one pipe) or four (two a bank, or a square on the one pipe)
    if (kind > 0.5 && kind < 3.5)
    {
        float spin = u_time * (1.5 + 30.0 * boost) * alive;
        float cnt = kind > 2.5 ? 4.0 : (kind > 1.5 ? 2.0 : 1.0);
        float rt = (0.05 + 0.02 * tb) * (cnt > 2.5 ? 0.55 : (cnt > 1.5 ? 0.8 : 1.0));
        vec2 t0 = vec2(two ? xm : xc + 0.06, yOut);
        vec2 t1 = t0;
        vec2 t2 = t0;
        vec2 t3 = t0;
        if (dual)
        {
            if (cnt < 1.5)
            {
                t0 = vec2(xc + 0.07, yC);
                col = ink(col, min(seg(p, vec2(xc, yT), t0), seg(p, vec2(xc, yB), t0)), pw * 0.8, pipeCol);
            }
            else
            {
                float sx = cnt > 2.5 ? 2.3 * rt : 0.0;
                t0 = vec2(xc + 0.05, yT);
                t1 = vec2(xc + 0.05, yB);
                t2 = vec2(xc + 0.05 + sx, yT);
                t3 = vec2(xc + 0.05 + sx, yB);
            }
        }
        else if (two)
        {
            // on each bank's pipe as it runs to where they meet
            vec2 m0 = vec2(xm, yC);
            float f0 = cnt > 2.5 ? 0.3 : 0.5;
            t0 = cnt < 1.5 ? m0 : mix(vec2(xc, yT), m0, f0);
            t1 = mix(vec2(xc, yB), m0, f0);
            t2 = mix(vec2(xc, yT), m0, 0.78);
            t3 = mix(vec2(xc, yB), m0, 0.78);
        }
        else if (cnt > 1.5)
        {
            float s = 1.1 * rt;
            t0 = vec2(xc + 0.06, yOut + s);
            t1 = vec2(xc + 0.06, yOut - s);
            t2 = t0;
            t3 = t1;
            if (cnt > 2.5)
            {
                t0 = vec2(xc + 0.02 + s, yOut + s);
                t1 = vec2(xc + 0.02 + s, yOut - s);
                t2 = vec2(xc + 0.02 + 3.0 * s, yOut + s);
                t3 = vec2(xc + 0.02 + 3.0 * s, yOut - s);
            }
        }
        col = turbo(col, p, t0, rt, spin, boost);
        if (cnt > 1.5)
            col = turbo(col, p, t1, rt, spin * 1.03, boost);
        if (cnt > 2.5)
        {
            col = turbo(col, p, t2, rt, spin * 0.97, boost);
            col = turbo(col, p, t3, rt, spin * 1.05, boost);
        }
    }

    // ---- the intake: the plenum and the throttle ----
    {
        float ph = two ? 0.035 : 0.04;
        float px0 = xs - 0.05;
        float px1 = xe + 0.05;
        float dpl = box(p, vec2(0.5 * (px0 + px1), yP), vec2(0.5 * (px1 - px0), ph), 0.02);
        col = mix(col, vec3(0.05, 0.065, 0.085), smoothstep(0.003, -0.003, dpl));
        col = ink(col, abs(dpl), 0.005, dimSteel * 1.3);
        // a supercharger on it, belted to the crank
        if (kind > 3.5)
            col = blower(col, p, vec2(0.5 * (px0 + px1), yP), min(0.046, 0.32 * (px1 - px0)), fract(max(in1, 0.0)) * 12.5663706, boost);
        // the throttle body at its end, its plate turning open with the throttle
        vec2 tbc = vec2(px0 - 0.055, yP);
        float dtb = length(p - tbc);
        col = mix(col, vec3(0.05, 0.065, 0.085), smoothstep(0.036, 0.032, dtb));
        col = ink(col, abs(dtb - 0.034), 0.005, steel);
        float pa = 1.5707963 * (1.0 - thr);
        vec2 pd = vec2(cos(pa), sin(pa)) * 0.03;
        col = ink(col, seg(p, tbc - pd, tbc + pd), 0.006, mix(steel, cold, thr * alive));
        col = ink(col, seg(p, vec2(px0 - 0.02, yP), vec2(px0, yP)), 0.008, dimSteel * 1.3);
    }

    // ---- a rotary ----
    if (fam > 2.5)
    {
        for (int rr = 0; rr < 2; ++rr)
        {
            float r = float(rr);
            vec2 rc = vec2(xs + r * pitch, yC);
            // its housing
            vec2 hq = (p - rc) / (vec2(0.15, 0.115) * rsz);
            float dh = (length(hq) - 1.0) * 0.115 * rsz;
            col = mix(col, vec3(0.05, 0.06, 0.075), smoothstep(0.003, -0.003, dh));
            col = ink(col, abs(dh), 0.005, steel);
            // its faces' burns: the plugs flash
            float burn = 0.0;
            float ex = 0.0;
            for (int ff = 0; ff < 3; ++ff)
            {
                float fa = fireAt(kk, r * 3.0 + float(ff));
                float delta = mod(A - fa, cyc);
                burn += exp(-delta / 70.0);
                ex = max(ex, step(190.0, delta) * step(delta, 590.0) * (1.0 - (delta - 190.0) / 400.0));
            }
            burn *= burning * load;
            vec2 plug = rc + vec2(0.135 * rsz, 0.0);
            float dpg = min(length(p - plug - vec2(0.0, 0.03 * rsz)), length(p - plug + vec2(0.0, 0.03 * rsz)));
            col = mix(col, mix(vec3(0.3), vec3(1.0, 0.9, 0.6), clamp(burn, 0.0, 1.0)), smoothstep(0.012, 0.008, dpg));
            col += vec3(1.0, 0.6, 0.2) * burn * 0.35 * exp(-length(p - plug) / 0.05);
            // the rotor: a third of the shaft's speed, the second 180 degrees of the shaft behind
            float ra = radians(A / 3.0 + r * 60.0);
            vec2 ecc = vec2(cos(radians(A + r * 180.0)), sin(radians(A + r * 180.0))) * 0.012 * rsz;
            float dr = tri(p - rc - ecc, 0.098 * rsz, ra) - 0.012 * rsz;
            col = mix(col, vec3(0.10, 0.12, 0.15), smoothstep(0.003, -0.003, dr));
            col = ink(col, abs(dr), 0.005, vec3(0.75, 0.80, 0.88));
            // its ports: exhaust below to the collector, intake above to the plenum
            vec2 eport = rc + vec2(-0.04, -0.115) * rsz;
            vec2 iport = rc + vec2(-0.04, 0.115) * rsz;
            col = ink(col, seg(p, eport, vec2(eport.x, yB)), 0.010, pipeCol);
            col = ink(col, seg(p, iport, vec2(iport.x, yP - 0.03)), 0.008, dimSteel * 1.3);
            col = mix(col, mix(vec3(0.25, 0.27, 0.30), hot, ex * alive), smoothstep(0.016, 0.011, length(p - eport)));
            // its pulses
            for (int ff = 0; ff < 3; ++ff)
            {
                float fa = fireAt(kk, r * 3.0 + float(ff));
                float delta = mod(A - fa, cyc);
                if (delta > 190.0 && delta < 640.0)
                {
                    float t = (delta - 190.0) * 0.0042;
                    vec2 pos = along(eport, vec2(eport.x, yB), vec2(exitX, yB), vec2(exitX, yB), t);
                    float fade = 1.0 - (delta - 190.0) / 450.0;
                    vec3 pc = mix(vec3(0.55, 0.62, 0.72), hot, step(0.75, burning));
                    col += pc * fade * alive * load * 0.9 * exp(-length(p - pos) / 0.011);
                }
            }
        }
    }
    else
    {
        // ---- the crank and the flywheel (under the bores of an inline; under the plenum of a vee or a flat) ----
        float fx = xs - 0.5 * pitch - 0.02;
        if (!two)
            col = ink(col, seg(p, vec2(fx, yC), vec2(xe + 0.4 * pitch, yC)), 0.006, dimSteel);
        float dfw = box(p, vec2(fx - 0.03, yC), vec2(0.016, 0.13), 0.01);
        col = mix(col, vec3(0.07, 0.08, 0.10), smoothstep(0.003, -0.003, dfw));
        col = ink(col, abs(dfw), 0.004, steel * 0.9);
        float ca = radians(mod(A, 360.0));
        float my = yC + 0.11 * cos(ca);
        col = mix(col, vec3(1.0, 0.85, 0.5), step(0.0, sin(ca)) * smoothstep(0.012, 0.004, length(p - vec2(fx - 0.03, my))) * alive);

        // ---- the cylinders ----
        float c0 = 0.0;
        float c1 = 0.0;
        for (int ii = 0; ii < 12; ++ii)
        {
            float i = float(ii);
            if (i > n - 0.5)
                break;
            float b = bankOf(mask, i);
            float j = b < 0.5 ? c0 : c1;
            if (b < 0.5)
                c0 += 1.0;
            else
                c1 += 1.0;
            vec2 cc = vec2(xs + j * pitch + (b > 0.5 ? stag * pitch : 0.0), yC + (b < 0.5 ? yOff : -yOff));
            // the exhaust's side: down for an inline and the lower bank, up for the upper
            float exDir = (!two || b > 0.5) ? -1.0 : 1.0;
            float yCol = exDir < 0.0 ? yB : yT;
            float fa = fireAt(kk, i);
            float delta = mod(A - fa, cyc);
            // its header and runner
            vec2 ve = cc + vec2(0.0, exDir * 0.45 * rb);
            vec2 vi = cc - vec2(0.0, exDir * 0.45 * rb);
            col = ink(col, seg(p, ve, vec2(cc.x, yCol)), 0.008, pipeCol);
            float yRun = two ? yP + sign(cc.y - yP) * 0.035 : yP - 0.04;
            col = ink(col, seg(p, vi, vec2(cc.x, yRun)), 0.007, dimSteel * 1.3);
            // the bore
            float dB = length(p - cc);
            col = mix(col, vec3(0.045, 0.055, 0.070), smoothstep(rb, rb - 0.004, dB));
            col = ink(col, abs(dB - rb), 0.005, steel);
            // the burn
            float burn = burning * exp(-delta / 55.0) * load;
            vec3 fl = mix(vec3(1.0, 0.96, 0.78), hot, clamp(delta / 70.0, 0.0, 1.0));
            col += fl * burn * smoothstep(rb, 0.0, dB) * 1.5;
            col += fl * burn * 0.22 * exp(-dB / (0.7 * rb));
            // the valves
            float exOpen = step(130.0, delta) * step(delta, 370.0);
            float inOpen = step(345.0, delta) * step(delta, 590.0);
            col = mix(col, mix(vec3(0.24, 0.27, 0.31), hot, exOpen * alive), smoothstep(0.30 * rb, 0.22 * rb, length(p - ve)));
            col = mix(col, mix(vec3(0.24, 0.27, 0.31), cold, inOpen * alive), smoothstep(0.27 * rb, 0.20 * rb, length(p - vi)));
            // the air drawn in down the runner
            if (delta > 345.0 && delta < 590.0)
            {
                float t = (delta - 345.0) / 245.0;
                vec2 pos = mix(vec2(cc.x, yRun), vi, t);
                col += cold * alive * (0.3 + 0.7 * thr) * 0.5 * (1.0 - t) * exp(-length(p - pos) / 0.008);
            }
            // the pulse: down the header, along the collector, out
            if (delta > 130.0 && delta < 620.0)
            {
                float t = (delta - 130.0) * 0.0045;
                vec2 c1p = vec2(cc.x, yCol);
                vec2 c2p = vec2(xc, yCol);
                bool straight = dual || !two;
                vec2 c3p = mix(vec2(xm, yC), vec2(exitX, yCol), straight ? 1.0 : 0.0);
                vec2 pos = along(ve, c1p, c2p, c3p, t);
                float l123 = length(c1p - ve) + length(c2p - c1p) + length(c3p - c2p);
                if (!straight && t > l123)
                    pos = vec2(min(xm + (t - l123), exitX), yC);
                float fade = 1.0 - (delta - 130.0) / 490.0;
                vec3 pc = mix(vec3(0.55, 0.62, 0.72), hot, step(0.75, burning));
                col += pc * fade * alive * load * 0.9 * exp(-length(p - pos) / 0.010);
            }
        }
    }

    // ---- the firing order, the cylinder that fired last lit ----
    {
        float u = min(0.0085, (W - 0.25) / (4.0 * (13.0 + 3.0 * n)));
        vec2 to = vec2(bx0 - 0.10, -0.40);
        float cell = floor((p.x - to.x) / (4.0 * u));
        if (p.y < to.y + 0.001 && p.y > to.y - 5.0 * u - 0.001 && cell >= 0.0)
        {
            vec2 o = vec2(to.x + cell * 4.0 * u, to.y);
            float bits = 0.0;
            if (cell < 12.5)
            {
                // FIRING ORDER
                bits = cell < 0.5 ? G_F : (cell < 1.5 ? G_I : (cell < 2.5 ? G_R : (cell < 3.5 ? G_I : (cell < 4.5 ? G_N :
                      (cell < 5.5 ? G_G : (cell < 6.5 ? 0.0 : (cell < 7.5 ? G_O : (cell < 8.5 ? G_R : (cell < 9.5 ? G_D :
                      (cell < 10.5 ? G_E : (cell < 11.5 ? G_R : 0.0)))))))))));
                col = mix(col, vec3(0.42, 0.48, 0.56), glyph(p, o, u, bits));
            }
            else
            {
                float e = floor((cell - 13.0) / 3.0);
                float slot = cell - 13.0 - e * 3.0;
                if (e >= 0.0 && e < n - 0.5)
                {
                    // the e-th to fire, and the one that fired last
                    float which = 0.0;
                    float last = 0.0;
                    float best = 1.0e4;
                    for (int ii = 0; ii < 12; ++ii)
                    {
                        float i = float(ii);
                        if (i > n - 0.5)
                            break;
                        float fi = fireAt(kk, i);
                        float rank = 0.0;
                        for (int jj = 0; jj < 12; ++jj)
                        {
                            float j = float(jj);
                            if (j > n - 0.5)
                                break;
                            if (fireAt(kk, j) < fi)
                                rank += 1.0;
                        }
                        if (abs(rank - e) < 0.5)
                            which = i;
                        float dl = mod(A - fi, cyc);
                        if (dl < best)
                        {
                            best = dl;
                            last = i;
                        }
                    }
                    float num = which + 1.0;
                    if (slot < 0.5)
                        bits = num > 9.5 ? digitGlyph(1.0) : 0.0;
                    else if (slot < 1.5)
                        bits = digitGlyph(num > 9.5 ? num - 10.0 : num);
                    else
                        bits = e < n - 1.5 ? G_MINUS : 0.0;
                    bool lit = abs(which - last) < 0.5 && alive > 0.25 && slot < 1.5;
                    col = mix(col, mix(vec3(0.70, 0.76, 0.84), hot, lit ? 1.0 : 0.0), glyph(p, o, u, bits));
                }
            }
        }
    }

    gl_FragColor = vec4(col, 1.0);
}
