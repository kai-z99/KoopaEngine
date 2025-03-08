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
    
    struct DirLight {
        vec3 direction;
        vec3 color;
        float intensity;
        bool isActive;    
    };

    vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
    uniform DirLight dirLight;

    in vec2 TexCoords;
    in vec3 ourNormal;
    in vec3 FragPos;

    uniform sampler2D currentDiffuse;
    uniform vec3 viewPos;

    void main()
    {
        vec3 color = vec3(0.0f);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 normal = normalize(ourNormal);

        for (int i = 0; i < 4; i++)
        {
            color += CalcPointLight(pointLights[i], normal, FragPos, viewDir);
            color += CalcDirLight(dirLight, normal, FragPos, viewDir);
        }
    
        float gamma = 2.2;

        //Ambient lighting
        vec3 sceneAmbient = vec3(0.015f, 0.015f, 0.015f) * pow(texture(currentDiffuse, TexCoords).rgb, vec3(gamma)); //degamma
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
        vec3 diffuseColor = pow(texture(currentDiffuse, TexCoords).rgb, vec3(gamma)); //degamma
    
        vec3 diffuse  = light.intensity * light.color  * diff * diffuseColor;
        vec3 specular = light.intensity * light.color  * spec; // * vec3(texture(material.specular, TexCoord)); not using map rn

        diffuse  *= attenuation;
        specular *= attenuation;

        return (diffuse + specular);
    } 

    vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
    {
        if (light.isActive == false)
        {
            return vec3(0.0f, 0.0f, 0.0f);
        }

        vec3 direction = normalize(light.direction);

        //diffuse
        float diff = max(dot(direction, normal), 0.0f);

        //spec
        vec3 halfwayDir = normalize(direction + viewDir);
        float spec;
        spec = pow(max(dot(halfwayDir, normal), 0.0), 64.0f); //64 = material shininess

        // combine results
        float gamma = 2.2;
        vec3 diffuseColor = pow(texture(currentDiffuse, TexCoords).rgb, vec3(gamma)); //degamma
    
        vec3 diffuse  = light.intensity * light.color  * diff * diffuseColor;
        vec3 specular = light.intensity * light.color  * spec; // * vec3(texture(material.specular, TexCoord)); not using map rn

        return (diffuse + specular);
    }