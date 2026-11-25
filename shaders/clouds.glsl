// Shared cloud layer, pulled in with #include "clouds.glsl".
// A flat sheet of fractal noise at CLOUD_HEIGHT in world space, drifting with the wind.
// The sky, the moon and the ground shadows all sample the same sheet, so they agree.

uniform vec2 cloudOffset;    // wind drift in world units
uniform float cloudCoverage; // 0 = clear, 1 = overcast

const float CLOUD_HEIGHT = 120.0;
const float CLOUD_SCALE = 1.0 / 90.0; // world units -> noise units (features ~40-90 units)

float cloudHash(ivec2 c)
{
	uvec2 v = uvec2(c) * uvec2(1664525u, 22695477u) + uvec2(1013904223u, 1u);
	uint h = v.x ^ (v.y * 747796405u);
	h ^= h >> 16u;
	h *= 2654435769u;
	h ^= h >> 15u;
	return float(h & 0xFFFFu) / 65535.0;
}

float cloudValueNoise(vec2 p)
{
	ivec2 i = ivec2(floor(p));
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	float a = cloudHash(i);
	float b = cloudHash(i + ivec2(1, 0));
	float c = cloudHash(i + ivec2(0, 1));
	float d = cloudHash(i + ivec2(1, 1));
	return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float cloudFbm(vec2 p)
{
	const mat2 turn = mat2(0.8, -0.6, 0.6, 0.8); // rotate each octave to hide grid artefacts
	float sum = 0.0, amp = 0.5;
	for (int i = 0; i < 5; i++) {
		sum += amp * cloudValueNoise(p);
		p = turn * p * 2.03;
		amp *= 0.5;
	}
	return sum;
}

// Cloud density (0..1) at a world XZ position on the sheet.
float cloudDensityAt(vec2 xz)
{
	float n = cloudFbm((xz + cloudOffset) * CLOUD_SCALE);
	float threshold = mix(0.72, 0.28, cloudCoverage);
	return smoothstep(threshold, threshold + 0.22, n);
}

// Where a ray from origin along dir crosses the sheet; returns false if it never does.
bool cloudHit(vec3 origin, vec3 dir, out vec2 xz)
{
	if (dir.y <= 0.001 || origin.y >= CLOUD_HEIGHT)
		return false;
	xz = origin.xz + dir.xz * ((CLOUD_HEIGHT - origin.y) / dir.y);
	return true;
}

// Density seen along a ray, faded towards the horizon where the sheet is far away and aliases.
float cloudDensityAlong(vec3 origin, vec3 dir)
{
	vec2 xz;
	if (!cloudHit(origin, dir, xz))
		return 0.0;
	return cloudDensityAt(xz) * smoothstep(0.03, 0.25, dir.y);
}
