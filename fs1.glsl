
    #version 330 core
    out vec4 FragColor;

    struct PointLight {    
        vec3 position;   
        vec3 color; 
        float intensity;
        bool isActive;
    };  

    vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor);
    uniform PointLight pointLights[4];
    uniform int numPointLights;
    
    struct DirLight {
        vec3 direction;
        vec3 color;
        float intensity;
        bool isActive;    
        bool castShadows;
    };

    vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor);
    uniform DirLight dirLight;
    
    float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
    float PointShadowCalculation(vec3 fragPos, vec3 lightPos);

    in vec2 TexCoords;
    in vec3 Normal;
    in vec3 FragPos;
    in mat3 TBN;
    in vec4 FragPosLightSpace;

    uniform sampler2D currentDiffuse;   //0
    uniform sampler2D currentNormalMap; //1
    uniform sampler2D dirShadowMap;     //2
    uniform samplerCube pointShadowMap;   //3
    uniform float farPlane; //for point shadow calculation
    uniform vec3 baseColor; //Use this if not using a diffuse texture
    uniform vec3 viewPos;

    uniform bool usingNormalMap;
    uniform bool usingDiffuseMap;

    void main()
    {
        
        vec3 color = vec3(0.0f);
        vec3 viewDir = normalize(viewPos - FragPos);
        float gamma = 2.2;
        vec3 diffuseColor = (usingDiffuseMap) ? pow(texture(currentDiffuse, TexCoords).rgb, vec3(gamma)) : baseColor;
    
        vec3 normalFromData = normalize(Normal);
        vec3 normalFromMap  = texture(currentNormalMap, TexCoords).rgb;
        normalFromMap = normalFromMap * 2.0f - 1.0f; //[0,1] -> [-1, 1]
        normalFromMap = normalize(TBN * normalFromMap); //Tangent -> World (tbn is constucted with model matrix)
        
        //float dirShadow = CalculateShadow(FragPosLightSpace, (usingNormalMap) ? normalFromMap : normalFromData, dirLight.direction);
        

        for (int i = 0; i < numPointLights; i++)
        {
            color += CalcPointLight(pointLights[i], (usingNormalMap) ? normalFromMap : normalFromData, FragPos, viewDir);
        }

        color += CalcDirLight(dirLight, (usingNormalMap) ? normalFromMap : normalFromData, FragPos, viewDir);
    
        //Ambient lighting
        vec3 sceneAmbient = vec3(0.115f, 0.115f, 0.115f) * diffuseColor; 
        color += sceneAmbient;

        color = pow(color, vec3(1.0f / gamma)); //Gamma correction

        FragColor = vec4(color, 1.0f);
    }
    
    float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
    {
        //note: we only care about z comp of fragPosLightSpace.
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; //perspective divide to convert to [-1,1]. 
        //Now: {[-1,1], [-1,1], [-1,1]} (if in frustum, AKA will show in the shadow snapshot, not clipped out)

        projCoords = projCoords * 0.5 + 0.5;  
        //Now: {[0,1] , [0,1], [0,1]} (if in frustum, AKA will show in the shadow snapshot, not clipped out)

        float closestDepth = texture(dirShadowMap, projCoords.xy).r; //note: this can sample outside [0,1] if its outsidde the frag was outside frustum. (clipped out)
        //if it does, it return 1.0f due to the wrapping method we set.
        float fragDepth = projCoords.z;

        if (fragDepth > 1.0f) return 0.0f; //Outside the far plane

        float bias = max(0.0025 * (1.0 - dot(normal, lightDir)), 0.005); //mote bias with more angle.
        float shadow = 0.0f;

        //PCF---
        vec2 texelSize = 1.0f / textureSize(dirShadowMap, 0);
        //This is like a convolution matrix.
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(dirShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; //closest depth of the surronding pixel
                shadow += fragDepth - bias > pcfDepth ? 1.0 : 0.0;   //if fragdepth > closest depth of near pixel, add more shadow  
                //note: think of what happen ShadowCalcultion samples a pixel thats on the edge of a box. This convolution will add some contribution
                //for the texels that are on the box, but wont add contribution to the ones that are off the edge of the box. making softer shadow. (its an edge as it should be)
                //Visualization: convert to lightSpace, 9 lasers in a square at the pixel being sampled. Lasers that hit before are add shadow,
                //lasers that hit behind sample pixel dont contribute to shadow.
            }    
        }
        shadow /= 9.0;
    
        //shadow = fragDepth - bias > closestDepth ? 1.0f : 0.0f;

        return shadow;  
    }

    float PointShadowCalculation(vec3 fragPos, vec3 lightPos, vec3 normal)
    {
        vec3 lightToFrag = fragPos - lightPos;

        float fragDepth = length(lightToFrag); // [0, farPlane]
        float closestDepth = texture(pointShadowMap, lightToFrag).r;// [0,1]
        closestDepth *= farPlane; //[0,1] -> [0, farPlane]
        
        float shadow = 0.0f;
        float bias = max(0.05 * (1.0 - dot(normalize(normal), normalize(lightToFrag))), 0.005);  
        
        if (fragDepth - bias > closestDepth) shadow += 1.0f;

        return shadow;
    }

    vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
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
        
        vec3 diffuse  = light.intensity * light.color  * diff * diffuseColor;
        vec3 specular = light.intensity * light.color  * spec; // * vec3(texture(material.specular, TexCoord)); not using map rn

        diffuse  *= attenuation;
        specular *= attenuation;
        
        float shadow = PointShadowCalculation(fragPos, light.position, normal);

        return (1.0f - shadow) * (diffuse + specular);
    } 

    vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
    {
        if (light.isActive == false)
        {
            return vec3(0.0f, 0.0f, 0.0f);
        }

        vec3 direction = -normalize(light.direction);

        //diffuse
        float diff = max(dot(direction, normal), 0.0f);

        //spec
        vec3 halfwayDir = normalize(direction + viewDir);
        float spec;
        spec = pow(max(dot(halfwayDir, normal), 0.0), 64.0f); //64 = material shininess

        // combine results
    
        vec3 diffuse  = light.intensity * light.color  * diff * diffuseColor;
        vec3 specular = light.intensity * light.color  * spec; // * vec3(texture(material.specular, TexCoord)); not using map rn
        
        if (!light.castShadows)
        {
            return (diffuse + specular);
        }
        else
        {
            float shadow = ShadowCalculation(FragPosLightSpace, normal, -direction);
            return (1.0 - shadow) * (diffuse + specular);
        }
    }