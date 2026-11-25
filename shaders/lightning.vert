#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec2 vEdgeIntensity; // x = -1..1 across the ribbon, y = branch intensity

uniform mat4 view;
uniform mat4 projection;

out vec2 edgeIntensity;

void main()
{
	edgeIntensity = vEdgeIntensity;
	gl_Position = projection * view * vec4(vPosition, 1.0);
}
