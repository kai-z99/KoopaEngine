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

    for (int i = 0; i < 4; i++)
    {
        color += CalcPointLight(pointLights[i], ourNormal, FragPos, viewDir);
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
    
    vec3 diffuse  = light.color  * diff * diffuseColor;
    vec3 specular = light.color  * spec; // * vec3(texture(material.specular, TexCoord)); not using map rn

    //ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (diffuse + specular);
} 