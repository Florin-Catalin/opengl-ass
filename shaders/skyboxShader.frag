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

// Rainbow: one bow centred on the antisolar point (opposite the sun), red outside, with a brighter
// sky inside it. Real rainbows sit at 42 degrees; this one is drawn larger and wider (RAINBOW_*)
// so it reads well on screen.
uniform vec3 sunDirWorld;
uniform float rainbowStrength; // 0 = none; set from the time since the rain stopped and the sun height
uniform float auroraStrength;  // 0..1: northern lights (clear night over full snow cover)
uniform float sandAmount;      // 0..1 sandstorm: dust hides the sky, thickest at the horizon
uniform vec3 dustColor;        // matches the scene fog colour, so the ground fades into the sky
uniform vec2 dustOffset;       // how far the dust billows have blown
const float RAINBOW_RADIUS = 48.0; // degrees from the antisolar point to the red outer edge
const float RAINBOW_WIDTH = 4.5;   // degrees from violet to red

vec3 spectrum(float t) // t: 0 = violet .. 1 = red
{
	vec3 c = vec3(smoothstep(0.5, 0.8, t) + 0.3 * smoothstep(0.12, 0.0, t),   // red (and violet's red)
	              smoothstep(0.25, 0.5, t) * smoothstep(0.95, 0.6, t),            // green
	              0.85 * smoothstep(0.5, 0.15, t));                               // blue
	return c;
}

vec3 rainbow(vec3 dir)
{
	float angle = degrees(acos(clamp(dot(dir, -sunDirWorld), -1.0, 1.0)));
	vec3 light = vec3(0.0);

	float inner = RAINBOW_RADIUS - RAINBOW_WIDTH;
	float p = (angle - inner) / RAINBOW_WIDTH; // 0 = violet edge .. 1 = red edge
	float band = smoothstep(0.0, 0.15, p) * smoothstep(1.0, 0.85, p);
	light += spectrum(clamp(p, 0.0, 1.0)) * band;

	// inside the bow the sky is lighter
	light += vec3(0.12) * smoothstep(inner, inner - 12.0, angle);
	return light;
}

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

// Aurora borealis: two wavy curtains low in the northern sky (towards -z). The ray is projected
// onto a stack of horizontal layers between 100 and 190 units up, so each curtain has a sharp,
// green lower edge and fades upwards into red-purple, with fine vertical rays drifting along it.
vec3 aurora(vec3 dir)
{
	if (dir.y < 0.02)
		return vec3(0.0);
	const int LAYERS = 26;
	vec3 sum = vec3(0.0);
	// A per-pixel offset between layers hides the steps between them (dithering).
	float jitter = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
	for (int i = 0; i < LAYERS; i++) {
		float f = (float(i) + jitter) / float(LAYERS); // 0 = bottom of the curtain, 1 = top
		vec2 p = dir.xz / dir.y * (100.0 + f * 90.0);
		vec3 tint = mix(vec3(0.15, 1.0, 0.45), vec3(0.75, 0.2, 0.85), smoothstep(0.3, 0.95, f));
		float fade = smoothstep(0.0, 0.1, f) * pow(max(1.0 - f, 0.0), 1.5); // max: i/(n-1) can round above 1
		for (int k = 0; k < 2; k++) {
			float fk = float(k);
			float centre = -230.0 - 120.0 * fk;
			float sway = 70.0 * sin(p.x * 0.008 + time * 0.07 + fk * 2.0)
			           + 110.0 * (cloudFbm(vec2(p.x * 0.0025 + fk * 7.0, time * 0.02)) - 0.5);
			float band = exp(-abs(p.y - centre - sway) / (5.0 + 4.0 * fk));
			float rays = cloudValueNoise(vec2(p.x * 0.12 + time * 0.35 + fk * 31.0, f * 1.2 + fk));
			rays = 0.15 + 0.85 * rays * rays * 1.6; // sharp bright rays with dimmer gaps
			sum += tint * band * rays * fade * (1.0 - 0.35 * fk);
		}
	}
	// It slowly brightens and fades, like a real display.
	float pulse = 0.75 + 0.25 * sin(time * 0.4) * sin(time * 0.17 + 1.0);
	return sum * (6.0 / float(LAYERS)) * pulse;
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

	if (auroraStrength > 0.0)
		color.rgb += aurora(dir) * auroraStrength * smoothstep(0.0, 0.12, dir.y);

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

	// The rainbow forms in the falling drops between you and the clouds, so it sits in front of them.
	if (rainbowStrength > 0.0)
		color.rgb = max(color.rgb + rainbow(dir) * rainbowStrength * horizonFade, vec3(0.0));

	// Sandstorm: billowing dust in front of everything. Solid at the horizon (where it meets the
	// dust fog over the ground), a little thinner and lumpier overhead. The sun is a pale disc.
	if (sandAmount > 0.0) {
		vec2 q = dir.xz / (max(dir.y, 0.0) + 0.3) * 2.5 + dustOffset * 0.04;
		float billow = cloudFbm(q);
		float up = smoothstep(0.05, 0.8, dir.y);
		float dust = sandAmount * mix(1.0, 0.8 + 0.25 * billow, up);
		vec3 dustSky = dustColor * mix(1.0, 0.65 + 0.7 * billow, up);
		color.rgb = mix(color.rgb, dustSky, clamp(dust, 0.0, 1.0));

		float s = dot(dir, sunDirWorld);
		float sunUp = smoothstep(-0.05, 0.08, sunDirWorld.y);
		float disc = smoothstep(cos(0.05), cos(0.04), s);
		float glow = pow(max(s, 0.0), 24.0);
		color.rgb += (vec3(1.0, 0.92, 0.75) * disc * 0.45 + dustColor * glow * 0.5) * sandAmount * sunUp;
	}
}
