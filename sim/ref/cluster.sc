// cluster.sc: the dash. Two dials, as a car has: the tachometer at the left, to this engine's own redline (numbers in
// thousands, red from the redline on, the rpm in digits under the hub), and the speedometer at the right (0 to 180 mph,
// the speed in digits under its hub), each with a small gauge in the gap at its bottom: the coolant's temperature (100
// to 260 F, its reading in digits) in the tachometer, the oil's pressure (0 to 100 psi) in the speedometer. Between
// them: shift lights that fill toward the redline and flash at the limiter, the gear, a vacuum-and-boost gauge for the
// manifold, the pedals as bars, and the warning lights.
// in0 rpm, in1 the redline (rpm), in2 the gear (-2 played from the keys, -1 R, 0 N, 1 to 18), in3 km/h (shown in mph),
// in4 the throttle (0 to 1), in5 the manifold against the air outside (bar: vacuum below 0, boost above), in6 the lamps
// (the state, 0 off 1 cranking 2 running 3 running down, + 4 limiter + 8 fuel cut + 16 driven; + 32 times the
// coolant's temperature, degrees F + 60, 0 to 511; + 16384 times the oil's pressure, psi, 0 to 127; + 2097152 driven as a
// truck), in7 the pedals (the
// brake 0 to 100 + 1000 times the clutch 0 to 100)
//
// (This compiler rejects a select between two colours: a 0-or-1 made by ?: that then blends two colours is folded into
// one. So 0-or-1 values here are made with step(), and colours are blended with mix(), never picked with ?:.)

const float G_A = 11245.0;
const float G_B = 27566.0;
const float G_C = 14627.0;
const float G_E = 31143.0;
const float G_H = 23533.0;
const float G_I = 29847.0;
const float G_K = 23469.0;
const float G_L = 18727.0;
const float G_M = 24557.0;
const float G_N = 27501.0;
const float G_O = 11114.0;
const float G_P = 27556.0;
const float G_R = 27565.0;
const float G_S = 14478.0;
const float G_T = 29842.0;
const float G_U = 23407.0;
const float G_V = 23402.0;
const float G_Y = 23186.0;
const float G_F = 31140.0;
const float G_X = 23213.0;
const float G_SL = 4772.0;
const float G_0 = 31599.0;
const float G_1 = 11415.0;
const float G_MINUS = 448.0;
const float G_PLUS = 1488.0;

float ang2(float y, float x)
{
    float a = atan(y / (abs(x) + 0.00001));
    if (x < 0.0)
        a = y >= 0.0 ? 3.14159265 - a : -3.14159265 - a;
    return a;
}

float seg(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

// bit d (1, 2, 4 ...) of the mask m
float bit(float m, float d)
{
    return mod(floor(m / d), 2.0);
}

// a glyph of a 3 by 5 dot font (its rows top to bottom, 3 bits each, the left the highest), its top left at o, a dot
// u wide
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

float word2(vec2 p, vec2 o, float u, float a, float b)
{
    return max(glyph(p, o, u, a), glyph(p, o + vec2(4.0 * u, 0.0), u, b));
}

float word3(vec2 p, vec2 o, float u, float a, float b, float c)
{
    return max(word2(p, o, u, a, b), glyph(p, o + vec2(8.0 * u, 0.0), u, c));
}

float word4(vec2 p, vec2 o, float u, float a, float b, float c, float d)
{
    return max(word3(p, o, u, a, b, c), glyph(p, o + vec2(12.0 * u, 0.0), u, d));
}

float word5(vec2 p, vec2 o, float u, float a, float b, float c, float d, float e)
{
    return max(word4(p, o, u, a, b, c, d), glyph(p, o + vec2(16.0 * u, 0.0), u, e));
}

// a seven-segment digit centred at c, h high, slanted as a dash's; mask: the segments a b c d e f g as bits 1 to 64.
// x: lit, y: every segment (for the unlit ones' ghosts)
vec2 digit7(vec2 p, vec2 c, float h, float mask)
{
    vec2 q = (p - c) / h;
    if (abs(q.y) > 0.62 || abs(q.x) > 0.46)
        return vec2(0.0);
    q.x -= q.y * 0.12;
    float w = 0.075;
    float w2 = w * 0.45;
    float sa = smoothstep(w, w2, seg(q, vec2(-0.19, 0.5), vec2(0.19, 0.5)));
    float sb = smoothstep(w, w2, seg(q, vec2(0.26, 0.42), vec2(0.26, 0.08)));
    float sc = smoothstep(w, w2, seg(q, vec2(0.26, -0.08), vec2(0.26, -0.42)));
    float sd = smoothstep(w, w2, seg(q, vec2(-0.19, -0.5), vec2(0.19, -0.5)));
    float se = smoothstep(w, w2, seg(q, vec2(-0.26, -0.08), vec2(-0.26, -0.42)));
    float sf = smoothstep(w, w2, seg(q, vec2(-0.26, 0.42), vec2(-0.26, 0.08)));
    float sg = smoothstep(w, w2, seg(q, vec2(-0.19, 0.0), vec2(0.19, 0.0)));
    float on = max(max(max(sa * bit(mask, 1.0), sb * bit(mask, 2.0)), max(sc * bit(mask, 4.0), sd * bit(mask, 8.0))),
                   max(max(se * bit(mask, 16.0), sf * bit(mask, 32.0)), sg * bit(mask, 64.0)));
    float full = max(max(max(sa, sb), max(sc, sd)), max(max(se, sf), sg));
    return vec2(on, full);
}

float digitMask(float d)
{
    if (d < 0.5)
        return 63.0;
    if (d < 1.5)
        return 6.0;
    if (d < 2.5)
        return 91.0;
    if (d < 3.5)
        return 79.0;
    if (d < 4.5)
        return 102.0;
    if (d < 5.5)
        return 109.0;
    if (d < 6.5)
        return 125.0;
    if (d < 7.5)
        return 7.0;
    if (d < 8.5)
        return 127.0;
    return 111.0;
}

float tenTo(float k)
{
    if (k < 0.5)
        return 1.0;
    if (k < 1.5)
        return 10.0;
    if (k < 2.5)
        return 100.0;
    if (k < 3.5)
        return 1000.0;
    return 10000.0;
}

// a whole number in up to n seven-segment digits, its last digit centred at c, h high; the leading zeros unlit
vec2 number7(vec2 p, vec2 c, float h, float value, float n)
{
    float v = floor(max(value, 0.0) + 0.5);
    float k = floor((c.x + 0.31 * h - p.x) / (0.62 * h));
    if (k < 0.0 || k > n - 1.0)
        return vec2(0.0);
    float pw = tenTo(k);
    float d = mod(floor((v + 0.5) / pw), 10.0);
    vec2 r = digit7(p, vec2(c.x - k * 0.62 * h, c.y), h, digitMask(d));
    r.x *= 1.0 - step(0.5, k) * (1.0 - step(pw, v));
    return r;
}

// where along a dial's sweep (0 at its start, 1 at its end) the angle a (degrees) lies; the sweep runs clockwise
// from 225 to -45 degrees
float sweepOf(float a)
{
    float t = 225.0 - a;
    if (t < 0.0)
        t += 360.0;
    if (t > 360.0)
        t -= 360.0;
    return t / 270.0;
}

vec2 dirOf(float s)
{
    float a = radians(225.0 - 270.0 * s);
    return vec2(cos(a), sin(a));
}

// a needle from the hub at c, r long, at sweep s
vec3 needle(vec3 col, vec2 p, vec2 c, float r, float s, vec3 tint)
{
    vec2 dir = dirOf(s);
    float nd = seg(p, c - dir * r * 0.16, c + dir * r * 0.92);
    col = mix(col, tint, smoothstep(0.022 * r, 0.009 * r, nd));
    col += tint * 0.30 * exp(-nd / (0.045 * r));
    float d = length(p - c);
    col = mix(col, vec3(0.10, 0.10, 0.11), smoothstep(0.095 * r, 0.08 * r, d));
    col += vec3(0.22, 0.22, 0.24) * smoothstep(0.012 * r, 0.0, abs(d - 0.088 * r));
    return col;
}

// a small gauge in a dial's bottom gap: its pivot at c, its needle r long, sweeping 120 degrees (from 150 to 30) over
// a scale from lo to hi at v; a red band from w0 to w1 on its scale; a tick each quarter
vec3 subGauge(vec3 col, vec2 p, vec2 c, float r, float v, float lo, float hi, float w0, float w1)
{
    vec2 q = p - c;
    float d = length(q);
    float s = (150.0 - degrees(ang2(q.y, q.x))) / 120.0;
    float inArc = step(0.0, s) * step(s, 1.0) * step(0.0, q.y);
    col = mix(col, vec3(0.30, 0.31, 0.34), inArc * smoothstep(0.035 * r, 0.015 * r, abs(d - r)));
    float b0 = (w0 - lo) / (hi - lo);
    float b1 = (w1 - lo) / (hi - lo);
    col = mix(col, vec3(0.85, 0.12, 0.08), inArc * step(b0, s) * step(s, b1) * smoothstep(0.07 * r, 0.04 * r, abs(d - r)));
    float tk = abs(s * 4.0 - floor(s * 4.0 + 0.5)) * 0.25 * 2.0944 * r;
    col = mix(col, vec3(0.80, 0.80, 0.78), inArc * smoothstep(0.03 * r, 0.012 * r, tk) * step(0.82 * r, d) * step(d, 1.08 * r));
    float sv = clamp((v - lo) / (hi - lo), -0.02, 1.02);
    float na = radians(150.0 - 120.0 * sv);
    vec2 nd = vec2(cos(na), sin(na));
    float sd = seg(p, c - nd * 0.12 * r, c + nd * 0.95 * r);
    col = mix(col, vec3(1.0, 0.42, 0.10), smoothstep(0.045 * r, 0.02 * r, sd));
    col += vec3(1.0, 0.42, 0.10) * 0.25 * exp(-sd / (0.06 * r));
    col = mix(col, vec3(0.10, 0.10, 0.11), smoothstep(0.11 * r, 0.09 * r, d));
    return col;
}

// a dial's face and bezel
vec3 face(vec3 col, vec2 p, vec2 c, float r)
{
    float d = length(p - c);
    float inside = smoothstep(r * 1.01, r * 0.99, d);
    vec3 f = vec3(0.050, 0.052, 0.058) + vec3(0.018) * (1.0 - d / r);
    col = mix(col, f, inside);
    col += vec3(0.30, 0.30, 0.32) * smoothstep(0.014 * r, 0.0, abs(d - r * 1.012));
    col += vec3(0.07, 0.07, 0.08) * smoothstep(0.03 * r, 0.0, abs(d - r * 1.05));
    return col;
}

void main()
{
    vec2 p = (v_px - 0.5 * u_size) / u_size.y;
    p.y = -p.y;
    float aspect = u_size.x / u_size.y;
    // across: the tachometer, the middle (0.74 wide), the speedometer; k scales it all to the height, s spreads the
    // width left over evenly
    float k = min(1.0, aspect / 2.5);
    float s = max((aspect - 2.46 * k) / 4.0, 0.0);
    float left = -0.5 * aspect;

    float rpm = max(in0, 0.0);
    float red = max(in1, 1500.0);
    float gear = in2;
    float mph = max(in3, 0.0) * 0.621371;
    float thr = clamp(in4, 0.0, 1.0);
    float mapBar = in5;
    float lampsIn = max(in6, 0.0);
    float truck = floor(lampsIn / 2097152.0 + 0.00001);
    float lampsAll = lampsIn - 2097152.0 * truck;
    float oilPsi = floor(lampsAll / 16384.0 + 0.00001);
    float rest6 = lampsAll - 16384.0 * oilPsi;
    float tempQ = floor(rest6 / 32.0 + 0.00001);
    float lamps = rest6 - 32.0 * tempQ;
    float tempF = tempQ - 60.0;
    float st = mod(lamps, 4.0);
    float lim = mod(floor(lamps / 4.0), 2.0);
    float cut = mod(floor(lamps / 8.0), 2.0);
    float drv = step(0.5, floor(lamps / 16.0));
    float ped = max(in7, 0.0);
    float brk = clamp(mod(ped, 1000.0) / 100.0, 0.0, 1.0);
    float clu = clamp(floor(ped / 1000.0) / 100.0, 0.0, 1.0);
    float alive = step(0.5, st);

    // the ground: dark, a faint carbon weave, a little warm light from below
    vec3 col = vec3(0.028, 0.030, 0.034);
    vec2 cw = floor(v_px / 3.0);
    col += 0.007 * mod(cw.x + cw.y, 2.0);
    col += vec3(0.05, 0.025, 0.008) * smoothstep(0.1, -0.6, p.y) * (0.4 + 0.6 * thr * alive);

    vec3 amber = vec3(1.0, 0.55, 0.14);
    vec3 dimText = vec3(0.42, 0.43, 0.46);
    float R = 0.41 * k;
    vec2 tc = vec2(left + s + 0.43 * k, -0.005);
    vec2 dc = vec2(tc.x + 0.43 * k + s + 0.37 * k, 0.0);
    vec2 vc = vec2(dc.x + 0.37 * k + s + 0.43 * k, -0.005);

    // ---- the tachometer ----
    float tmax = ceil(red * 1.12 / 1000.0);
    float every = tmax > 10.5 ? 2.0 : 1.0;
    vec2 q = p - tc;
    float d = length(q);
    if (d < R * 1.1)
    {
        col = face(col, p, tc, R);
        float sw = sweepOf(degrees(ang2(q.y, q.x)));
        float redS = red / (tmax * 1000.0);
        {
            float inDial = step(sw, 1.0) * step(d, R);
            float v = sw * tmax;
            float arc = R * 4.712389 / tmax;
            float fm = abs(v - floor(v + 0.5)) * arc;
            float fh = abs(v * 2.0 - floor(v * 2.0 + 0.5)) * 0.5 * arc;
            float major = inDial * smoothstep(0.014 * R, 0.006 * R, fm) * step(R * 0.79, d) * step(d, R * 0.95);
            float minor = inDial * smoothstep(0.009 * R, 0.003 * R, fh) * step(R * 0.87, d) * step(d, R * 0.95);
            vec3 tcol = mix(vec3(0.86, 0.86, 0.82), vec3(1.0, 0.22, 0.14), step(redS, sw));
            col = mix(col, tcol, max(major, 0.6 * minor));
            float band = inDial * step(redS, sw) * smoothstep(0.03 * R, 0.022 * R, abs(d - R * 0.965));
            col = mix(col, vec3(0.78, 0.07, 0.05), band);
            float nv = clamp(rpm / (tmax * 1000.0), 0.0, 1.0);
            float lit = inDial * step(sw, nv) * smoothstep(0.022 * R, 0.0, abs(d - R * 0.745));
            col += lit * mix(amber, vec3(1.0, 0.15, 0.08), step(red, rpm)) * 0.85;
        }
        // the numbers, in thousands: the nearest one to this point (in the gap at the bottom, the half nearer the
        // start belongs to the 0, the rest to the last number)
        float swn = sweepOf(degrees(ang2(q.y, q.x)));
        float sn = swn > 1.1667 ? 0.0 : min(swn, 1.0);
        float m = floor(sn * tmax / every + 0.5) * every;
        if (m <= tmax + 0.01)
        {
            float hN = 0.15 * R;
            float nd = m > 9.5 ? 2.0 : 1.0;
            vec2 nc = tc + dirOf(m / tmax) * R * 0.63;
            vec2 r7 = number7(p, nc + vec2((nd - 1.0) * 0.31 * hN, 0.0), hN, m, nd);
            vec3 ncol = mix(vec3(0.90, 0.90, 0.86), vec3(1.0, 0.25, 0.16), step(red, m * 1000.0));
            col = mix(col, ncol, r7.x);
        }
        // the rpm in digits under the hub, and its name
        float hD = 0.13 * R;
        vec2 dr = number7(p, tc + vec2(2.0 * 0.62 * hD, -0.43 * R), hD, rpm, 5.0);
        col = mix(col, vec3(0.10, 0.06, 0.03), dr.y * 0.6);
        col = mix(col, amber, dr.x);
        float u = 0.021 * R;
        float t1 = word5(p, tc + vec2(-17.5 * u, -0.22 * R), u, G_X, G_1, G_0, G_0, G_0);
        float t2 = word3(p, tc + vec2(3.5 * u, -0.22 * R), u, G_R, G_P, G_M);
        col = mix(col, dimText, max(t1, t2));
        // the coolant's temperature in the gap at its bottom: C to H (100 to 260 F, red from 240), its reading in
        // digits over the pivot, TEMP and F beside it
        {
            vec2 gc = tc + vec2(0.0, -0.86 * R);
            float rg = 0.26 * R;
            float ug = 0.016 * R;
            col = mix(col, vec3(0.35, 0.60, 1.0), glyph(p, gc + vec2(-0.36 * R, 0.13 * R + 2.5 * ug), ug, G_C));
            col = mix(col, vec3(1.0, 0.30, 0.20), glyph(p, gc + vec2(0.30 * R, 0.13 * R + 2.5 * ug), ug, G_H));
            col = mix(col, dimText, word4(p, gc + vec2(-0.30 * R, -0.04 * R + 2.5 * ug), ug, G_T, G_E, G_M, G_P));
            col = mix(col, dimText, glyph(p, gc + vec2(0.07 * R, -0.04 * R + 2.5 * ug), ug, G_F));
            float hT = 0.075 * R;
            // (no reading before the engine has first run since the plugin loaded: the digits dark, the needle at C)
            float tOn = step(0.5, tempQ);
            vec2 tr = number7(p, gc + vec2(0.62 * hT, 0.13 * R), hT, abs(tempF), 3.0);
            col = mix(col, vec3(0.10, 0.06, 0.03), tr.y * 0.5);
            col = mix(col, mix(amber, vec3(1.0, 0.25, 0.16), step(240.0, tempF)), tOn * (tr.x * alive + tr.x * 0.4 * (1.0 - alive)));
            col = mix(col, amber, tOn * step(tempF, -0.5) * glyph(p, gc + vec2(-0.115 * R, 0.13 * R + 2.5 * ug), ug, G_MINUS));
            col = subGauge(col, p, gc, rg, tOn > 0.5 ? tempF : 0.0, 100.0, 260.0, 240.0, 260.0);
        }
        col = needle(col, p, tc, R, clamp(rpm / (tmax * 1000.0), -0.01, 1.03), vec3(1.0, 0.42, 0.10));
    }

    // ---- the speedometer: 0 to 180 mph, a mark every 10, a number every 20 ----
    vec2 vq = p - vc;
    float vd = length(vq);
    if (vd < R * 1.1)
    {
        col = face(col, p, vc, R);
        float sw = sweepOf(degrees(ang2(vq.y, vq.x)));
        {
            float inDial = step(sw, 1.0) * step(vd, R);
            float v = sw * 18.0;
            float arc = R * 4.712389 / 18.0;
            float fm = abs(v * 0.5 - floor(v * 0.5 + 0.5)) * 2.0 * arc;
            float fh = abs(v - floor(v + 0.5)) * arc;
            float major = inDial * smoothstep(0.014 * R, 0.006 * R, fm) * step(R * 0.79, vd) * step(vd, R * 0.95);
            float minor = inDial * smoothstep(0.009 * R, 0.003 * R, fh) * step(R * 0.87, vd) * step(vd, R * 0.95);
            col = mix(col, vec3(0.86, 0.86, 0.82), max(major, 0.6 * minor));
            float nv = clamp(mph / 180.0, 0.0, 1.0);
            float lit = inDial * step(sw, nv) * smoothstep(0.022 * R, 0.0, abs(vd - R * 0.745));
            col += lit * amber * 0.85 * (0.35 + 0.65 * drv);
        }
        float swn = sweepOf(degrees(ang2(vq.y, vq.x)));
        float sn = swn > 1.1667 ? 0.0 : min(swn, 1.0);
        float m = floor(sn * 9.0 + 0.5) * 20.0;
        float hN = 0.12 * R;
        float nd = m > 99.5 ? 3.0 : (m > 9.5 ? 2.0 : 1.0);
        vec2 nc = vc + dirOf(m / 180.0) * R * 0.62;
        vec2 r7 = number7(p, nc + vec2((nd - 1.0) * 0.31 * hN, 0.0), hN, m, nd);
        col = mix(col, vec3(0.90, 0.90, 0.86), r7.x);
        // the speed in digits under the hub, and its unit
        float hD = 0.13 * R;
        vec2 dr = number7(p, vc + vec2(0.62 * hD, -0.43 * R), hD, mph, 3.0);
        col = mix(col, vec3(0.10, 0.06, 0.03), dr.y * 0.6);
        col = mix(col, mix(vec3(0.35, 0.35, 0.37), amber, drv), dr.x);
        float u = 0.021 * R;
        col = mix(col, dimText, word3(p, vc + vec2(-5.5 * u, -0.22 * R), u, G_M, G_P, G_H));
        // the oil's pressure in the gap at its bottom: 0 to 100 psi (red under 10), its reading in digits, OIL and PSI
        {
            vec2 gc = vc + vec2(0.0, -0.86 * R);
            float rg = 0.26 * R;
            float ug = 0.016 * R;
            col = mix(col, dimText, glyph(p, gc + vec2(-0.36 * R, 0.13 * R + 2.5 * ug), ug, G_0));
            col = mix(col, dimText, word3(p, gc + vec2(0.25 * R, 0.13 * R + 2.5 * ug), ug, G_1, G_0, G_0));
            col = mix(col, dimText, word3(p, gc + vec2(-0.26 * R, -0.04 * R + 2.5 * ug), ug, G_O, G_I, G_L));
            col = mix(col, dimText, word3(p, gc + vec2(0.07 * R, -0.04 * R + 2.5 * ug), ug, G_P, G_S, G_I));
            float hT = 0.075 * R;
            vec2 orr = number7(p, gc + vec2(0.31 * hT, 0.13 * R), hT, oilPsi, 3.0);
            col = mix(col, vec3(0.10, 0.06, 0.03), orr.y * 0.5);
            col = mix(col, mix(vec3(1.0, 0.25, 0.16), amber, min(step(10.0, oilPsi) + (1.0 - alive), 1.0)), orr.x * (0.4 + 0.6 * alive));
            col = subGauge(col, p, gc, rg, oilPsi, 0.0, 100.0, 0.0, 10.0);
        }
        col = needle(col, p, vc, R, clamp(mph / 180.0, -0.01, 1.03), vec3(1.0, 0.42, 0.10));
    }

    // ---- the middle ----
    // the shift lights: they fill from 80 % of the redline and flash at the limiter
    {
        float x0 = dc.x - 0.30 * k;
        float dx = 0.60 * k / 11.0;
        float i = clamp(floor((p.x - x0) / dx + 0.5), 0.0, 11.0);
        vec2 lc = vec2(x0 + i * dx, 0.45 * k);
        float ld = length(p - lc);
        if (ld < 0.05 * k)
        {
            float on = step(red * (0.80 + 0.018 * i), rpm);
            vec3 c = mix(mix(vec3(1.0, 0.12, 0.08), vec3(1.0, 0.78, 0.10), step(i, 7.5)), vec3(0.15, 1.0, 0.32), step(i, 3.5));
            float flash = step(0.5, lim);
            on = mix(on, step(0.5, fract(u_time * 7.0)), flash);
            c = mix(c, vec3(0.30, 0.55, 1.0), flash);
            col = mix(col, c * 0.10 + vec3(0.02), smoothstep(0.019 * k, 0.015 * k, ld));
            col = mix(col, c, on * smoothstep(0.019 * k, 0.012 * k, ld));
            col += c * on * 0.30 * exp(-ld / (0.025 * k));
        }
    }
    // the gear (R, N, 1 to 6 driven as a car, to 18 as a truck, in two digits from 10; a dash when the keys play it), CAR,
    // SEMI or KEYS over it
    {
        vec2 gc = vec2(dc.x - 0.19 * k, 0.17 * k);
        float hg = 0.22 * k;
        vec2 gq = abs(p - gc) - vec2(0.12 * k, 0.18 * k);
        float box = length(max(gq, 0.0)) + min(max(gq.x, gq.y), 0.0) - 0.03 * k;
        col = mix(col, vec3(0.015, 0.016, 0.018), smoothstep(0.004, -0.004, box));
        col += vec3(0.20, 0.20, 0.22) * smoothstep(0.006, 0.0, abs(box));
        float gm = 64.0;
        if (drv > 0.5)
            gm = gear < -0.5 ? 80.0 : (gear < 0.5 ? 84.0 : digitMask(gear));
        vec2 g7 = digit7(p, gc - vec2(0.0, 0.03 * k), hg, gm);
        if (drv > 0.5 && gear > 9.5)
            g7 = number7(p, gc + vec2(0.048 * k, -0.03 * k), 0.155 * k, gear, 2.0);
        col = mix(col, vec3(0.07, 0.04, 0.02), g7.y * 0.7);
        vec3 gcol = mix(vec3(0.35, 0.35, 0.37), amber, drv);
        col = mix(col, gcol, g7.x);
        col += gcol * 0.10 * g7.x;
        float u = 0.0085 * k;
        float w = drv > 0.5 ? (truck > 0.5 ? word4(p, gc + vec2(-7.5 * u, 0.16 * k), u, G_S, G_E, G_M, G_I)
                                           : word3(p, gc + vec2(-5.5 * u, 0.16 * k), u, G_C, G_A, G_R))
                            : word4(p, gc + vec2(-7.5 * u, 0.16 * k), u, G_K, G_E, G_Y, G_S);
        col = mix(col, dimText, w);
    }
    // the manifold: vacuum and boost, -1 to +1.5 bar (0 at 0.4 of the sweep), beside the gear
    {
        float Rb = 0.165 * k;
        vec2 bc = vec2(dc.x + 0.185 * k, 0.17 * k);
        vec2 bq = p - bc;
        float bd = length(bq);
        col = face(col, p, bc, Rb);
        float sw = sweepOf(degrees(ang2(bq.y, bq.x)));
        float inDial = step(sw, 1.0) * step(bd, Rb);
        float v = sw * 2.5 - 1.0;
        float arc = Rb * 4.712389 / 2.5;
        float fm = abs(v * 2.0 - floor(v * 2.0 + 0.5)) * 0.5 * arc;
        float major = inDial * smoothstep(0.022 * Rb, 0.010 * Rb, fm) * step(Rb * 0.78, bd) * step(bd, Rb * 0.95);
        col = mix(col, mix(vec3(0.70, 0.78, 0.90), vec3(1.0, 0.75, 0.35), 1.0 - step(v, 0.0)), major);
        float mv = clamp((mapBar + 1.0) / 2.5, 0.0, 1.0);
        float lo = min(mv, 0.4);
        float hi = max(mv, 0.4);
        float lit = inDial * step(lo, sw) * step(sw, hi) * smoothstep(0.04 * Rb, 0.0, abs(bd - Rb * 0.70));
        col += lit * mix(vec3(0.35, 0.60, 1.0), vec3(1.0, 0.62, 0.15), 1.0 - step(mapBar, 0.0)) * 0.85;
        float u = 0.0055 * k;
        col = mix(col, vec3(0.55, 0.65, 0.80), word3(p, bc + vec2(-0.72 * Rb, -0.48 * Rb), u, G_V, G_A, G_C));
        col = mix(col, vec3(1.0, 0.70, 0.35), word5(p, bc + vec2(0.72 * Rb - 19.0 * u, -0.48 * Rb), u, G_B, G_O, G_O, G_S, G_T));
        col = mix(col, vec3(0.75), glyph(p, bc + dirOf(0.4) * Rb * 0.56 + vec2(-1.5 * u, 2.5 * u), u, G_0));
        col = needle(col, p, bc, Rb, clamp((mapBar + 1.0) / 2.5, -0.01, 1.03), vec3(1.0, 0.50, 0.15));
    }
    // the pedals: throttle, brake, clutch
    {
        float u = 0.0085 * k;
        float x0 = dc.x - 0.22 * k;
        float x1 = dc.x + 0.33 * k;
        for (int i = 0; i < 3; ++i)
        {
            float fi = float(i);
            float y = -0.10 * k - fi * 0.055 * k;
            float val = i == 0 ? thr : (i == 1 ? brk : clu);
            vec3 c = mix(mix(vec3(0.30, 0.62, 1.0), vec3(1.0, 0.25, 0.18), step(fi, 1.5)), vec3(0.25, 1.0, 0.40), step(fi, 0.5));
            float on = mix(drv, 1.0, step(fi, 0.5));
            float lab = i == 0 ? word3(p, vec2(dc.x - 0.33 * k, y + 2.5 * u), u, G_T, G_H, G_R)
                               : (i == 1 ? word3(p, vec2(dc.x - 0.33 * k, y + 2.5 * u), u, G_B, G_R, G_K)
                                         : word3(p, vec2(dc.x - 0.33 * k, y + 2.5 * u), u, G_C, G_L, G_U));
            col = mix(col, dimText * (0.5 + 0.5 * on), lab);
            float inY = step(abs(p.y - y), 0.016 * k);
            float inX = step(x0, p.x) * step(p.x, x1);
            col = mix(col, vec3(0.06, 0.06, 0.07), inY * inX);
            float fill = step(p.x, x0 + (x1 - x0) * val) * step(abs(p.y - y), 0.011 * k) * inX;
            col = mix(col, c * (0.35 + 0.65 * on), fill);
        }
    }
    // the warning lights: cranking, running, the limiter, the fuel cut
    {
        float u = 0.0105 * k;
        vec2 o1 = vec2(dc.x - 0.21 * k, -0.29 * k);
        vec2 o2 = vec2(dc.x + 0.09 * k, -0.29 * k);
        vec2 o3 = vec2(dc.x - 0.21 * k, -0.38 * k);
        vec2 o4 = vec2(dc.x + 0.09 * k, -0.38 * k);
        float a1 = step(0.5, st) * (1.0 - step(1.5, st));
        float a2 = step(1.5, st);
        float l1 = word5(p, o1, u, G_S, G_T, G_A, G_R, G_T);
        float l2 = word3(p, o2, u, G_R, G_U, G_N);
        float l3 = word5(p, o3, u, G_L, G_I, G_M, G_I, G_T);
        float l4 = word3(p, o4, u, G_C, G_U, G_T);
        vec3 off = vec3(0.16, 0.16, 0.18);
        col = mix(col, mix(off, vec3(1.0, 0.72, 0.15), a1), l1);
        col = mix(col, mix(off, vec3(0.25, 1.0, 0.40), a2), l2);
        col = mix(col, mix(off, vec3(1.0, 0.18, 0.12), lim), l3);
        col = mix(col, mix(off, vec3(1.0, 0.60, 0.15), cut * (1.0 - lim)), l4);
        // their glow
        col += vec3(1.0, 0.72, 0.15) * a1 * 0.10 * exp(-length(p - o1 - vec2(9.5 * u, -2.5 * u)) / (0.05 * k));
        col += vec3(0.25, 1.0, 0.40) * a2 * 0.08 * exp(-length(p - o2 - vec2(5.5 * u, -2.5 * u)) / (0.05 * k));
        col += vec3(1.0, 0.18, 0.12) * lim * 0.12 * exp(-length(p - o3 - vec2(9.5 * u, -2.5 * u)) / (0.05 * k));
    }

    gl_FragColor = vec4(col, 1.0);
}
