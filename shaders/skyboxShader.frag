#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;
uniform vec3 skyTint;

#include "clouds.glsl"
uniform vec3 cameraPosWorld;
uniform vec3 lightDirWorld;    // towards the sun by day, the moon by night (unit length)
uniform vec3 cloudLitColor;    // cloud side facing the light
uniform vec3 cloudShadowColor; // cloud underside / thick parts

// Procedural stars (no texture): visible at night, turning with the sun/moon.
uniform float starVisibility; // 0 by day, 1 at full night
uniform mat3 starRotation;    // world direction -> star-sphere direction
uniform float time;           // seconds, for twinkling

// Planets: steady (non-twinkling) points. Brightness already includes twilight and sun-glare fades.
const int PLANET_COUNT = 4;
uniform vec3 planetDir[PLANET_COUNT];         // world direction
uniform vec3 planetColor[PLANET_COUNT];
uniform float planetBrightness[PLANET_COUNT]; // 0 = invisible, ~1.6 = Venus at its brightest

// Shooting stars (meteors), spawned with M. Each one is a short streak moving along a great
// circle: it starts at meteorStart and moves towards meteorTangent.
const int MAX_METEORS = 8;
uniform int meteorCount;
uniform vec3 meteorStart[MAX_METEORS];   // unit direction where it appears
uniform vec3 meteorTangent[MAX_METEORS]; // unit direction of travel at the start (perpendicular)
uniform vec4 meteorState[MAX_METEORS];   // x = head angle travelled, y = tail length (radians),
                                         // z = brightness, w = fireball tint (0 = white, 1 = green-orange)

// Moon halo: blue glow in the sky around the moon model, which is drawn after the skybox.
uniform vec3 moonDirWorld;
uniform float moonGlow; // 0 = moon down or daytime, 1 = full night with the moon up
const float MOON_ANGULAR_RADIUS = 0.04; // matches MOON_SCALE / MOON_DISTANCE in main.cpp
const vec3 MOONLIGHT_COLOR = vec3(0.45, 0.6, 1.0);

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

// A planet: a sharp disc a few pixels wide plus a soft glow that grows with brightness,
// which is how a bright planet reads to the eye. Distances are angles in radians.
vec3 planet(vec3 dir, int i)
{
	float b = planetBrightness[i];
	if (b <= 0.0)
		return vec3(0.0);
	float d = length(dir - planetDir[i]);
	float core = smoothstep(0.0025 + 0.003 * b, 0.0, d) * min(b, 1.0);
	float glow = exp(-d / (0.005 * b)) * 0.35 * b;
	return planetColor[i] * (core + glow);
}

// 0 right next to a visible planet, 1 away from all of them. Thins out the stars around each
// planet (about 3 degrees across) so the planet stands out.
float starMaskNearPlanets(vec3 dir)
{
	float mask = 1.0;
	for (int i = 0; i < PLANET_COUNT; i++) {
		float b = min(planetBrightness[i], 1.0);
		if (b <= 0.0)
			continue;
		float d = length(dir - planetDir[i]);
		mask = min(mask, mix(1.0, smoothstep(0.015, 0.05, d), b));
	}
	return mask;
}

vec3 meteor(vec3 dir, int i)
{
	vec3 a = meteorStart[i];
	vec3 t = meteorTangent[i];
	vec3 axis = cross(a, t);              // pole of the meteor's great circle
	float off = abs(dot(dir, axis));      // angular distance from the path
	if (off > 0.02)
		return vec3(0.0);
	float along = atan(dot(dir, t), dot(dir, a)); // angle along the path from the start

	float head = meteorState[i].x;
	float tail = meteorState[i].y;
	float behind = head - along;          // how far behind the head this pixel is
	if (behind < -0.01 || behind > tail)
		return vec3(0.0);

	float width = 0.0012 + 0.0008 * meteorState[i].z;
	float line = smoothstep(width, 0.0, off) * step(0.0, behind);
	float trail = pow(1.0 - clamp(behind / tail, 0.0, 1.0), 2.0); // brightest at the head
	float headGlow = exp(-length(vec2(off, behind)) / 0.004);

	vec3 tint = mix(vec3(0.85, 0.92, 1.0), vec3(0.6, 1.0, 0.55), meteorState[i].w);
	vec3 headColor = mix(vec3(1.0), vec3(1.0, 0.75, 0.4), meteorState[i].w);
	return meteorState[i].z * (tint * line * trail + headColor * headGlow * 0.8);
}

void main()
{
	color = texture(skybox, textureCoordinates) * vec4(skyTint, 1.0);

	vec3 dir = normalize(textureCoordinates);
	float horizonFade = smoothstep(-0.02, 0.15, dir.y); // haze near the horizon

	// Moon halo: a tight bright ring plus a wide faint wash of blue, and fewer stars near the moon.
	float moonStarMask = 1.0;
	if (moonGlow > 0.0) {
		float fromEdge = max(length(dir - moonDirWorld) - MOON_ANGULAR_RADIUS, 0.0);
		float halo = 0.35 * exp(-fromEdge / 0.015) + 0.12 * exp(-fromEdge / 0.12);
		color.rgb += MOONLIGHT_COLOR * halo * moonGlow * horizonFade;
		moonStarMask = mix(1.0, smoothstep(0.05, 0.3, fromEdge), moonGlow);
	}

	if (starVisibility > 0.0) {
		vec3 starDir = starRotation * dir;
		vec3 stars = starLayer(starDir, 90.0, 0.06, 0.22, 1.2)   // sparse, bright
		           + starLayer(starDir, 220.0, 0.05, 0.25, 0.6); // dense, faint
		color.rgb += stars * starVisibility * horizonFade * starMaskNearPlanets(dir) * moonStarMask;
	}

	vec3 planets = vec3(0.0);
	for (int i = 0; i < PLANET_COUNT; i++)
		planets += planet(dir, i);
	color.rgb += planets * horizonFade;

	vec3 meteors = vec3(0.0);
	for (int i = 0; i < meteorCount; i++)
		meteors += meteor(dir, i);
	color.rgb += meteors * horizonFade;

	// Clouds cover everything above (stars, planets, halo). Thick parts and parts with more cloud
	// between them and the light are darker; thin edges near the sun or moon get a bright lining.
	vec2 hit;
	if (cloudCoverage > 0.0 && cloudHit(cameraPosWorld, dir, hit)) {
		float density = cloudDensityAt(hit) * smoothstep(0.03, 0.25, dir.y);
		if (density > 0.0) {
			vec2 towardsLight = normalize(lightDirWorld.xz + vec2(1e-4));
			float occluded = cloudDensityAt(hit + towardsLight * 25.0);
			float lit = clamp(1.0 - 0.7 * occluded - 0.3 * density, 0.0, 1.0);
			vec3 cloudColor = mix(cloudShadowColor, cloudLitColor, lit);
			float forward = pow(max(dot(dir, lightDirWorld), 0.0), 12.0);
			cloudColor += cloudLitColor * forward * (1.0 - density) * 0.8;
			color.rgb = mix(color.rgb, cloudColor, density);
		}
	}
}
