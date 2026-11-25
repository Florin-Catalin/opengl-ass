#version 410 core

layout(location=0)
in vec3 vPosition;
layout(location=1)
in vec3 vNormal;
layout(location=2)
in vec2 vTexCoords;

out vec3 normal;
out vec4 fragPosEye;
out vec4 fragPosLightSpace;
out vec2 fragTexCoords;

out vec4 fragPos;
out vec3 normalWorld; // for puddles (model matrices are rotations, so mat3(model) is fine)
out vec3 posModel;    // puddle pattern stays fixed to the terrain when the scene rotates

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix;
uniform vec4 clipPlane; // world-space plane for the puddle reflection pass (GL_CLIP_DISTANCE0)

void main() 
{
	//compute eye space coordinates
	fragPosEye = view * model * vec4(vPosition, 1.0f);
	normal = vNormal;
	fragTexCoords = vTexCoords;
	fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);

	fragPos = model * vec4(vPosition, 1.0f);
	normalWorld = mat3(model) * vNormal;
	posModel = vPosition;
	gl_ClipDistance[0] = dot(fragPos, clipPlane);
}
