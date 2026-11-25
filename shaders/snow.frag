#version 410 core

in float flakeFade;
out vec4 fColor;

uniform vec3 snowColor; // lit by the current ambient/sun, like the rain

void main()
{
	float r = length(gl_PointCoord - vec2(0.5)) * 2.0; // 0 centre .. 1 edge
	if (r > 1.0)
		discard;
	float alpha = smoothstep(1.0, 0.3, r) * flakeFade * 0.9;
	fColor = vec4(snowColor, alpha);
}
