#version 410 core

layout(location=0) in vec4 seed; // all 0..1

#include "tornado.glsl"

uniform mat4 view;
uniform mat4 projection;
uniform float pixelsPerUnitAtOne;

out float puffAlpha;
out float isPuff;

void main()
{
	float t = tornadoTime;
	// Most particles are debris and dust whirling up around the foot; about 1 in 8 is a big
	// slow dust puff rolling along the ground around it.
	isPuff = step(seed.w, 0.12);
	float rise = fract(seed.y + t * mix(0.18, 0.06, isPuff) * (0.6 + 0.8 * seed.x));
	float h = rise * mix(14.0, 3.0, isPuff);
	float r = mix(1.5, 7.0, pow(seed.x, 0.7)) + h * 0.35 + isPuff * 4.0;
	r *= mix(0.4, 1.0, tornadoWidth);
	float omega = mix(10.0, 2.0, isPuff) / r;              // faster close to the core
	float a = seed.z * 6.2831853 + t * omega;
	vec2 axis = funnelAxis(h / funnelHeight);
	vec3 p = tornadoBase + vec3(axis.x + cos(a) * r, h, axis.y + sin(a) * r);

	puffAlpha = smoothstep(0.0, 0.1, rise) * (1.0 - smoothstep(0.6, 1.0, rise));
	vec4 eye = view * vec4(p, 1.0);
	float dist = max(-eye.z, 0.1);
	float size = mix(0.03 + 0.05 * seed.w, 3.5, isPuff);
	gl_PointSize = clamp(pixelsPerUnitAtOne * size / dist, 2.0, mix(6.0, 160.0, isPuff));
	gl_Position = projection * eye;
}
