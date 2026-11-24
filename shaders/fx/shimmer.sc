// shimmer.sc: heat shimmer above something hot (an exhaust's ends, a hot bonnet): faint bands that waver as they rise.
// A shader here cannot bend what is drawn under it (it reads nothing of the frame), so this is light only: a faint
// wavering haze, brightest low down, gone at the top. Take it away and no fact is lost.
// Inputs: in0 heat 0..1 (0 draws nothing), in1 a phase moved only by step(dt), in2..in4 the haze's colour (0..1 rgb).

void main()
{
    float h = clamp(in0, 0.0, 1.0);
    float y = v_uv.y;
    float wave = sin(v_uv.x * 18.0 + y * 7.0 - in1 * 6.2831853 * 2.0) * sin(y * 23.0 - in1 * 6.2831853 * 3.0 + v_uv.x * 5.0);
    float bands = clamp(wave * 0.5 + 0.5, 0.0, 1.0);
    float fade = y * clamp(1.0 - abs(v_uv.x * 2.0 - 1.0), 0.0, 1.0);   // low and central
    gl_FragColor = vec4(clamp(vec3(in2, in3, in4), 0.0, 1.0), h * bands * fade * 0.18);
}
