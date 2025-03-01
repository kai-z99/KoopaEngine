#pragma once

namespace ShaderSources
{
    const char* vs1 = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec2 TexCoords;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoords = aTexCoords;
    }
    )";

    const char* fs1 = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;
    
    uniform sampler2D currentTexture;

    void main()
    {
        //FragColor = vec4(0.0, 0.5, 0.2, 1.0);
        FragColor = texture(currentTexture, TexCoords);
    }
    )";
}
