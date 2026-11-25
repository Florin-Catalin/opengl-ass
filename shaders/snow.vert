#version 410 core

layout(location=0) in vec4 seed; // per flake: xyz = spot in the box (0..1), w = variation

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPosWorld;
uniform float time;
uniform vec3 snowVelocity;        // world units per second: a slow fall, drifted by the wind
uniform vec3 snowBoxSize;         // the box of snow around the camera
uniform float pixelsPerUnitAtOne; // projection scale: screen pixels per world unit at distance 1

out float flakeFade;

void main()
{
	float k = seed.w;
	// Like the rain: flakes fall forever and mod() wraps them into the box around the camera.
	vec3 travel = snowVelocity * time * (0.7 + 0.6 * k);
	vec3 p = mod(seed.xyz * snowBoxSize + travel - cameraPosWorld, snowBoxSize);
	p += cameraPosWorld - 0.5 * snowBoxSize;

	// Flakes flutter from side to side as they fall.
	float t = time * (0.8 + 0.8 * k) + seed.x * 40.0;
	p.x += 0.35 * sin(t) + 0.15 * sin(t * 2.3 + seed.y * 9.0);
	p.z += 0.35 * cos(t * 0.8 + seed.z * 20.0);

	vec3 rel = (p - cameraPosWorld) / (0.5 * snowBoxSize);
	float edge = max(max(abs(rel.x), abs(rel.y)), abs(rel.z));
	flakeFade = (1.0 - smoothstep(0.6, 1.0, edge)) * smoothstep(0.3, 1.2, length(p - cameraPosWorld));

	vec4 eye = view * vec4(p, 1.0);
	float dist = max(-eye.z, 0.1);
	gl_PointSize = clamp(pixelsPerUnitAtOne * 0.05 * (0.6 + 0.8 * k) / dist, 1.5, 20.0);
	gl_Position = projection * eye;
}
