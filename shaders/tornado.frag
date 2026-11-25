#version 410 core

in vec2 funnelUV;
in vec3 worldPos;
in vec3 radialDir;
out vec4 fColor;

#include "tornado.glsl"

uniform vec3 cameraPosWorld;
uniform vec3 lightDirWorld;
uniform vec3 lightColor;
uniform vec3 ambientLight;
uniform vec3 fogColor;
uniform float visibility; // 0..1 overall fade
uniform float extent;     // 0..1: how far down from the clouds the funnel reaches (touchdown)

// Value noise that repeats every `period` cells in x, so it wraps around the funnel seamlessly.
float hash(ivec2 c)
{
	uint h = uint(c.x) * 374761393u + uint(c.y) * 668265263u;
	h = (h ^ (h >> 13u)) * 1274126177u;
	return float(h & 0xFFFFu) / 65535.0;
}
float periodicNoise(vec2 p, int period)
{
	ivec2 i = ivec2(floor(p));
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	int x0 = ((i.x % period) + period) % period, x1 = (x0 + 1) % period;
	float a = hash(ivec2(x0, i.y)), b = hash(ivec2(x1, i.y));
	float c = hash(ivec2(x0, i.y + 1)), d = hash(ivec2(x1, i.y + 1));
	return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
float swirl(vec2 p)
{
	float sum = 0.0, amp = 0.5;
	int period = 8;
	for (int i = 0; i < 4; i++) {
		sum += amp * periodicNoise(p, period);
		p *= 2.0;
		period *= 2;
		amp *= 0.5;
	}
	return sum;
}

void main()
{
	float v = funnelUV.y;
	// Touchdown: the funnel grows down out of the cloud.
	float reach = 1.0 - extent;
	float body = smoothstep(reach, reach + 0.06, v);
	if (body <= 0.0)
		discard;

	// The pattern spins (fastest near the ground) and streams upwards.
	float spin = tornadoTime * 2.2 / (v + 0.25);
	vec2 p = vec2(funnelUV.x * 8.0 - spin, v * 18.0 - tornadoTime * 1.2);
	vec2 warp = vec2(swirl(p * 0.5 + vec2(0.0, 3.7)), swirl(p * 0.5 + vec2(0.0, 9.1)));
	float n = swirl(p + (warp - 0.5) * 3.0); // streaks bent and torn by a second noise
	float density = 0.5 + 0.6 * smoothstep(0.3, 0.7, n);
	bool outer = shellScale > 1.0;
	if (outer) // the outer shell is only torn streaks of dust, with sky showing between them
		density = 0.6 * smoothstep(0.45, 0.75, swirl(p * vec2(1.0, 0.6) + (warp - 0.5) * 4.0 + 5.0));

	// A tube of dust: seen edge-on (at the silhouette) we look through more of it.
	vec3 V = normalize(cameraPosWorld - worldPos);
	// Right at the silhouette it thins out again, so the outline is soft rather than a hard edge.
	float facing = abs(dot(V, radialDir));
	float alpha = density * mix(0.6, 1.0, 1.0 - facing) * smoothstep(0.0, 0.25, facing);
	alpha *= body * (1.0 - smoothstep(0.82, 1.0, v)); // blends into the cloud base at the top
	alpha *= visibility;

	// Dark grey, browner near the ground where it carries dirt; lit softly from the light side.
	vec3 base = mix(vec3(0.3, 0.24, 0.17), vec3(0.36, 0.37, 0.39), smoothstep(0.0, 0.4, v));
	float lit = 0.55 + 0.45 * max(dot(radialDir, normalize(vec3(lightDirWorld.x, 0.0, lightDirWorld.z) + 1e-4)), 0.0);
	vec3 color = base * (ambientLight * 1.4 + lightColor * 0.4 * lit) * (0.55 + 0.9 * n);

	// Distant haze.
	float d = length(cameraPosWorld - worldPos);
	color = mix(color, fogColor, 1.0 - exp(-pow(d * 0.006, 2.0)));
	fColor = vec4(color, clamp(alpha, 0.0, 1.0));
}
