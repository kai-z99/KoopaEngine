#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
    
uniform sampler2D currentTexture;

void main()
{
    //FragColor = vec4(0.0, 0.5, 0.2, 1.0);
    FragColor = texture(currentTexture, TexCoords);
}