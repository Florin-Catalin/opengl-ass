#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;
uniform vec3 skyTint;

// Procedural stars (no texture): visible at night, turning with the sun/moon.
uniform float starVisibility; // 0 by day, 1 at full night
uniform mat3 starRotation;    // world direction -> star-sphere direction
uniform float time;           // seconds, for twinkling

// Integer hash (PCG-style) -> 3 floats in [0, 1).
vec3 hash3(ivec3 c)
{
	uvec3 v = uvec3(c) * 1664525u + 1013904223u;
	v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z; v.y += v.z * v.x; v.z += v.x * v.y;
	return vec3(v & uvec3(0xFFFFu)) / 65536.0;
}

// One layer of stars: the star sphere is cut into a 3D grid of cells, and some cells hold
// one star at a random spot. Returns the star light at this direction.
vec3 starLayer(vec3 dir, float cells, float density, float radius, float brightness)
{
	vec3 p = dir * cells;
	ivec3 cell = ivec3(floor(p));
	vec3 rnd = hash3(cell);
	if (rnd.x > density)
		return vec3(0.0);

	vec3 starPos = vec3(cell) + 0.25 + 0.5 * hash3(cell + ivec3(17, 31, 47));
	float d = length(p - starPos);
	float core = smoothstep(radius, 0.0, d);
	float twinkle = 0.7 + 0.3 * sin(time * (1.5 + 3.0 * rnd.y) + rnd.z * 6.2832);
	vec3 tint = mix(vec3(0.75, 0.85, 1.0), vec3(1.0, 0.9, 0.75), rnd.y); // blue-white to warm
	return tint * core * brightness * twinkle * (0.5 + 0.5 * rnd.z);
}

void main()
{
	color = texture(skybox, textureCoordinates) * vec4(skyTint, 1.0);

	if (starVisibility > 0.0) {
		vec3 dir = normalize(textureCoordinates);
		vec3 starDir = starRotation * dir;
		vec3 stars = starLayer(starDir, 90.0, 0.06, 0.22, 1.2)   // sparse, bright
		           + starLayer(starDir, 220.0, 0.05, 0.25, 0.6); // dense, faint
		float horizonFade = smoothstep(-0.02, 0.15, dir.y);      // haze near the horizon
		color.rgb += stars * starVisibility * horizonFade;
	}
}
