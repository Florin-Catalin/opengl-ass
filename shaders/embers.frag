#version 410 core

in vec3 vColor;
in float vFlame;
out vec4 fColor;

void main()
{
	float r = length(gl_PointCoord - vec2(0.5)) * 2.0;
	if (r > 1.0)
		discard;
	// Sparks: a bright point. Flames: a soft blob, so overlapping ones blend into a fire.
	float shape = vFlame > 0.5 ? (1.0 - r) * (1.0 - r) : smoothstep(1.0, 0.2, r);
	fColor = vec4(vColor * shape, 1.0); // additive blending
}
