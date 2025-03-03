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
    out vec3 FragPos; //Frag pos in world position
    out vec3 ourNormal;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);

        TexCoords = aTexCoords;
        FragPos = vec3(model * vec4(aPos, 1.0));

        mat3 normalMatrix = mat3(transpose(inverse(model)));
        vec3 N = normalize(normalMatrix * aNormal);
        ourNormal = N;
    }
    )";

    const char* fs1 = R"(
    #version 330 core
    out vec4 FragColor;

    struct PointLight {    
        vec3 position;   
        vec3 color; 
        float intensity;
        bool isActive;
    };  

    vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
    uniform PointLight pointLights[4];

    in vec2 TexCoords;
    in vec3 ourNormal;
    in vec3 FragPos;

    uniform sampler2D currentTexture;
    uniform vec3 viewPos;

    void main()
    {
        vec3 color = vec3(0.0f);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 normal = normalize(ourNormal);

        for (int i = 0; i < 4; i++)
        {
            color += CalcPointLight(pointLights[i], normal, FragPos, viewDir);
        }
    
        float gamma = 2.2;

        //Ambient lighting
        vec3 sceneAmbient = vec3(0.015f, 0.015f, 0.015f) * pow(texture(currentTexture, TexCoords).rgb, vec3(gamma)); //degamma
        color += sceneAmbient;

        color = pow(color, vec3(1.0f / gamma)); //Gamma correction

        FragColor = vec4(color, 1.0f);
    }

    vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
    {
        //return normal;
        if (light.isActive == false)
        {
            return vec3(0.0f, 0.0f, 0.0f);
        }
        
        vec3 lightDir = normalize(light.position - fragPos);

        // diffuse shading
        float diff = max(dot(normal, lightDir), 0.0);

        // specular shading
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec;
        spec = pow(max(dot(halfwayDir, normal), 0.0), 64.0f);
    
        // attenuation
        float distance    = length(light.position - fragPos);
        float attenuation = 1.0 / (distance * distance); //quadratic attenuation

        // combine results
        float gamma = 2.2;
        vec3 diffuseColor = pow(texture(currentTexture, TexCoords).rgb, vec3(gamma)); //degamma
    
        vec3 diffuse  = light.intensity * light.color  * diff * diffuseColor;
        vec3 specular = light.intensity * light.color  * spec; // * vec3(texture(material.specular, TexCoord)); not using map rn

        diffuse  *= attenuation;
        specular *= attenuation;

        return (diffuse + specular);
    } 
    )";

    const char* fsLight = R"(
    #version 330 core
    out vec4 FragColor;
    
    uniform vec3 lightColor;

    void main()
    {
        FragColor = vec4(lightColor, 1.0); // a cube, that doesnt acutally emit light remember its purely a visual
        //FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    )";

}
