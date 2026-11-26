#version 410 core

in vec3 normalWorld;
in vec2 fragTexCoords;
in vec3 fragPosWorld;

out vec4 fColor;

uniform sampler2D diffuseTexture;
uniform float discPass;     // 0 = the hole (solid black), 1 = the glowing accretion disc
uniform float glowStrength;
uniform float visibility;   // hidden by overcast weather

#include "clouds.glsl"
uniform vec3 cameraPosWorld;

void main()
{
	// It is far beyond the clouds, so clouds in front of it hide it.
	float cloud = cloudDensityAlong(cameraPosWorld, normalize(fragPosWorld - cameraPosWorld));
	float alpha = visibility * (1.0 - cloud);
	if (discPass < 0.5) {
		fColor = vec4(0.0, 0.0, 0.0, alpha); // a black hole cut out of the sky
		return;
	}
	// The disc's texture is drawn on black, so it is added onto the sky (additive blending).
	fColor = vec4(texture(diffuseTexture, fragTexCoords).rgb * glowStrength, alpha);
}
