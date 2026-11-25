#version 410 core

in float dropFade;
out vec4 fColor;

uniform vec3 rainColor;  // follows the scene lighting (grey by day, dark blue at night)
uniform float rainAlpha; // overall strength, ramps with the rain amount

void main()
{
	fColor = vec4(rainColor, rainAlpha * dropFade);
}
