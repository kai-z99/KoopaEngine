#pragma once

namespace ShaderSources
{
    const char* vs1 = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    layout (location = 3) in vec3 aTangent;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 lightSpaceMatrix;

    out vec2 TexCoords;
    out vec3 FragPos; //Frag pos in world position
    out vec3 Normal;
    out mat3 TBN; //TangentSpace -> WorldSpace
    out vec4 FragPosLightSpace;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);

        TexCoords = aTexCoords;
        FragPos = vec3(model * vec4(aPos, 1.0));

        mat3 normalMatrix = mat3(transpose(inverse(model)));
        vec3 N = normalize(normalMatrix * aNormal);
        vec3 T = normalize(normalMatrix * aTangent);
        vec3 B = cross(N,T);
        TBN = mat3(T,B,N);

        Normal = N;

        FragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0);
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
    uniform int numPointLights;
    
    struct DirLight {
        vec3 direction;
        vec3 color;
        float intensity;
        bool isActive;    
        bool castShadows;
    };

    vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
    uniform DirLight dirLight;
    
    float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);

    in vec2 TexCoords;
    in vec3 Normal;
    in vec3 FragPos;
    in mat3 TBN;
    in vec4 FragPosLightSpace;

    uniform sampler2D currentDiffuse;   //0
    uniform sampler2D currentNormalMap; //1
    uniform sampler2D dirShadowMap;     //2
    uniform vec3 baseColor; //Use this if not using a diffuse texture
    uniform vec3 viewPos;

    uniform bool usingNormalMap;
    uniform bool usingDiffuseMap;

    void main()
    {
        
        vec3 color = vec3(0.0f);
        vec3 viewDir = normalize(viewPos - FragPos);
    
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
    
        float gamma = 2.2;

        //Ambient lighting
        vec3 sceneAmbient = vec3(0.015f, 0.015f, 0.015f) * pow(texture(currentDiffuse, TexCoords).rgb, vec3(gamma)); //degamma
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

        vec3 diffuseColor;
        
        if (usingDiffuseMap)
        {
            float gamma = 2.2;
            diffuseColor = pow(texture(currentDiffuse, TexCoords).rgb, vec3(gamma)); //degamma
        }
        else
        {
            diffuseColor = baseColor;
        }
        
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

        vec3 direction = -normalize(light.direction);

        //diffuse
        float diff = max(dot(direction, normal), 0.0f);

        //spec
        vec3 halfwayDir = normalize(direction + viewDir);
        float spec;
        spec = pow(max(dot(halfwayDir, normal), 0.0), 64.0f); //64 = material shininess

        // combine results
        vec3 diffuseColor;
        
        if (usingDiffuseMap)
        {
            float gamma = 2.2;
            diffuseColor = pow(texture(currentDiffuse, TexCoords).rgb, vec3(gamma)); //degamma
        }
        else
        {
            diffuseColor = baseColor;
        }
    
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

    const char* vsScreenQuad = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords;

    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
        TexCoords = aTexCoords;
    } 
    )";

    const char* fsScreenQuad = R"(
    #version 330 core
    out vec4 FragColor;
  
    in vec2 TexCoords;

    uniform sampler2D screenTexture;
    

    void main()
    {
        vec4 col = texture(screenTexture, TexCoords);  
        FragColor = col;
    }
    )";

    const char* vsDirShadow = R"(
    #version 330 core

    layout (location = 0) in vec3 aPos;

    uniform mat4 lightSpaceMatrix;
    uniform mat4 model;

    //This vertex shader simply converts a fragment to light space. Nothing else
    void main()
    {
        gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    }  
    )";

    const char* fsDirShadow = R"(
    #version 330 core

    void main()
    {
	    gl_FragDepth = gl_FragCoord.z; //set depth buffer manually if we want (this is done auto though so who cares...)
    }
    )";



}
