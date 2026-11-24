// tests/ui/panels/glow.sc: a small shader for the kit's tests: a disc whose size follows an input, a word of text.
#include "funk/text.sh"
#include "funk/shapes.sh"

void main()
{
    vec2 c = u_size * 0.5;
    float r = 10.0 + 40.0 * in_glow;
    float a = circle(v_px, c, r);
    vec3 ink = mix(vec3(0.1, 0.1, 0.12), vec3(1.0, 0.6, 0.2), a * (0.4 + 0.6 * in_out));
    ink = mix(ink, vec3(1.0), text(v_px, vec2(8.0, 20.0), 12.0, 0.0, "RPM"));
    gl_FragColor = vec4(ink, 1.0);
}
