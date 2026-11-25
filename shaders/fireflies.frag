#version 410 core

in float glow;
out vec4 fColor;

uniform float visibility; // night fade (0 by day and in the rain)

void main()
{
	// Round soft dot: bright core and a wider halo.
	float r = length(gl_PointCoord - vec2(0.5)) * 2.0; // 0 centre .. 1 edge
	if (r > 1.0)
		discard;
	float core = smoothstep(0.3, 0.0, r);
	float halo = (1.0 - r) * (1.0 - r) * 0.6;
	vec3 color = vec3(0.75, 1.0, 0.3) * (1.5 * core + halo) + vec3(0.7) * core * core;
	fColor = vec4(color * glow * visibility, 1.0); // additive blending
}
