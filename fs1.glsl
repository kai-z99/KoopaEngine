#version 410 core
layout (location = 0) out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skyboxTexture;

void main()
{
	FragColor = texture(skyboxTexture, TexCoords);
}