#version 410 core

in float puffAlpha;
in float isPuff;
out vec4 fColor;

uniform vec3 dustLight;   // ambient + some direct light
uniform float visibility;

void main()
{
	float r = length(gl_PointCoord - vec2(0.5)) * 2.0;
	if (r > 1.0)
		discard;
	// Debris: small dark specks. Puffs: big soft dust clouds.
	float shape = mix(smoothstep(1.0, 0.5, r), (1.0 - r) * (1.0 - r), isPuff);
	vec3 color = mix(vec3(0.18, 0.14, 0.1), vec3(0.55, 0.47, 0.37), isPuff) * dustLight;
	fColor = vec4(color, shape * puffAlpha * mix(1.0, 0.6, isPuff) * visibility);
}
