#version 410 core

layout(location=0) in vec4 seed; // xyz = home position in the area (0..1), w = personality

uniform mat4 view;
uniform mat4 projection;
uniform vec3 areaMin;
uniform vec3 areaSize;
uniform float time;
uniform float pixelsPerUnitAtOne; // projection scale: screen pixels per world unit at distance 1

out float glow;

void main()
{
	float k = seed.w;
	vec3 home = areaMin + seed.xyz * areaSize;

	// Lazy wandering loops around the home spot, each firefly at its own pace.
	float t = time * (0.25 + 0.25 * k);
	vec3 drift = vec3(sin(t * 1.3 + k * 17.0) * 1.2 + sin(t * 0.7 + seed.x * 9.0) * 0.6,
	                  sin(t * 0.9 + k * 5.0) * 0.35,
	                  cos(t * 1.1 + seed.z * 13.0) * 1.2 + cos(t * 0.5 + k * 3.0) * 0.6);
	vec3 p = home + drift;

	// Blinking: a short bright flash every 2-5 s, then dark, like real fireflies.
	float period = 2.0 + 3.0 * fract(k * 7.31);
	float phase = fract(time / period + k * 3.7);
	glow = smoothstep(0.0, 0.06, phase) * smoothstep(0.3, 0.1, phase);
	glow = max(glow, 0.15); // a faint ember between flashes

	vec4 eye = view * vec4(p, 1.0);
	float dist = max(-eye.z, 0.1);
	gl_PointSize = clamp(pixelsPerUnitAtOne * 0.3 / dist, 4.0, 60.0);
	gl_Position = projection * eye;
}
