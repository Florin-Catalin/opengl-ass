#version 410 core

layout(location=0) in vec4 seed; // x = lifetime, y = phase, z = identity, w = kind (all 0..1)

uniform mat4 view;
uniform mat4 projection;
uniform vec3 firePos;      // world position of the fire's base
uniform vec3 wind;         // world units per second that the sparks drift with
uniform float time;
uniform float strength;    // 0 = out .. 1 = burning: how many particles are alive
uniform float glow;        // overall brightness (embers stand out more at night)
uniform float pixelsPerUnitAtOne;

out vec3 vColor;
out float vFlame;

float hash(float n) { return fract(sin(n) * 43758.5453); }

void main()
{
	// About 1 in 4 particles is a short-lived flame tongue; the rest are sparks.
	vFlame = step(seed.w, 0.25);
	float life = vFlame > 0.5 ? 0.45 + 0.4 * seed.x : 1.6 + 2.2 * seed.x;
	float cycles = time / life + seed.y * 7.0;
	float age = fract(cycles);
	float n = floor(cycles) * 1.618 + seed.z * 113.0; // each rebirth gets new random numbers
	float r1 = hash(n), r2 = hash(n + 1.7), r3 = hash(n + 3.1), r4 = hash(n + 5.3);

	// A dying fire throws fewer particles; an extinguished one none.
	float alive = step(r4, strength);

	vec3 birth = firePos + vec3((r1 - 0.5) * 0.5, 0.0, (r2 - 0.5) * 0.5) * mix(1.0, 0.8, vFlame);
	vec3 p;
	float heat;
	if (vFlame > 0.5) {
		// Flames lick upwards half a unit and lean a little with the wind.
		p = birth + vec3(0.0, age * (0.4 + 0.3 * r3), 0.0) + wind * 0.04 * age;
		heat = 1.0 - age;
	} else {
		// Sparks shoot up, slow down, swirl in the hot air and get carried by the wind.
		float rise = (1.2 + 1.6 * r3) * life;
		float t = age * life;
		p = birth + vec3(0.0, rise * age * (1.0 - 0.45 * age), 0.0)
		      + wind * pow(t, 1.4) * 0.5
		      + vec3(sin(t * 6.0 + r1 * 20.0), 0.0, cos(t * 5.0 + r2 * 20.0)) * 0.18 * age;
		heat = 1.0 - age;
	}

	// Colour: flames are yellow fading to deep orange; sparks cool from yellow to red and go out.
	float flicker = 0.65 + 0.35 * sin(time * 30.0 + r1 * 50.0);
	vec3 hot = vec3(1.0, 0.78, 0.35), cool = vec3(0.7, 0.1, 0.0);
	if (vFlame > 0.5)
		vColor = mix(vec3(1.0, 0.35, 0.05), vec3(1.0, 0.8, 0.4), heat) * heat * 0.45;
	else
		vColor = mix(cool, hot, pow(heat, 1.5)) * flicker * smoothstep(0.0, 0.25, heat) * 1.3;
	vColor *= alive * glow * smoothstep(0.0, 0.05, age);

	vec4 eye = view * vec4(p, 1.0);
	float dist = max(-eye.z, 0.1);
	float size = vFlame > 0.5 ? 0.32 * (1.0 - 0.4 * age) : 0.03;
	gl_PointSize = clamp(pixelsPerUnitAtOne * size / dist, 1.5, vFlame > 0.5 ? 120.0 : 6.0);
	gl_Position = projection * eye;
}
