#version 410 core

in vec3 normalWorld;
in vec2 fragTexCoords;
in vec3 fragPosWorld;

out vec4 fColor;

uniform sampler2D diffuseTexture;
uniform vec3 sunDirWorld; // direction towards the sun; lights the moon, which gives it phases
uniform float moonAlpha;  // fades the moon in at dusk and out at dawn

#include "clouds.glsl"
uniform vec3 cameraPosWorld;

void main()
{
	vec3 albedo = texture(diffuseTexture, fragTexCoords).rgb;
	float lit = max(dot(normalize(normalWorld), normalize(sunDirWorld)), 0.0f);
	// Unlit side keeps a faint earthshine; the lit side is pushed bright so it reads as glowing.
	vec3 color = albedo * (0.25f + 1.6f * lit) + vec3(0.04f, 0.07f, 0.14f); // faint blue self-glow
	// The sky (with its clouds) is already drawn; let clouds in front of the moon show through.
	float cloud = cloudDensityAlong(cameraPosWorld, normalize(fragPosWorld - cameraPosWorld));
	fColor = vec4(min(color, vec3(1.0f)), moonAlpha * (1.0f - cloud));
}
