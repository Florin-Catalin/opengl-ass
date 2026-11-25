#version 410 core

layout(location=0) in vec2 uv; // u = around (0..1), v = up (0..1)

#include "tornado.glsl"

uniform mat4 view;
uniform mat4 projection;

out vec2 funnelUV;
out vec3 worldPos;
out vec3 radialDir;

void main()
{
	float a = uv.x * 6.2831853;
	float v = uv.y;
	radialDir = vec3(cos(a), 0.0, sin(a));
	vec2 axis = funnelAxis(v);
	// A ragged outline that turns with the spin, so the silhouette isn't a smooth tube.
	float ragged = 1.0 + 0.1 * sin(a * 3.0 + v * 23.0 - tornadoTime * 4.0) + 0.06 * sin(a * 7.0 - v * 41.0 - tornadoTime * 6.0);
	worldPos = tornadoBase + vec3(axis.x, v * funnelHeight, axis.y) + radialDir * funnelRadius(v) * ragged * shellScale;
	funnelUV = uv;
	gl_Position = projection * view * vec4(worldPos, 1.0);
}
