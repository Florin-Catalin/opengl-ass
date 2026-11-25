#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;
uniform vec3 skyTint;

void main()
{
    color = texture(skybox, textureCoordinates) * vec4(skyTint, 1.0);
}
