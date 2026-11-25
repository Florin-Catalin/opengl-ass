#version 410 core

in vec2 edgeIntensity;
out vec4 fColor;

uniform float boltBrightness; // follows the strike's flicker

void main()
{
	float e = abs(edgeIntensity.x);
	float core = smoothstep(0.25, 0.0, e);    // white-hot channel
	float glow = exp(-e * 3.0) * 0.45;        // blue-violet glow around it
	vec3 color = vec3(0.95, 0.97, 1.0) * core + vec3(0.55, 0.6, 1.0) * glow;
	fColor = vec4(color * edgeIntensity.y * boltBrightness, 1.0); // additive blending
}
