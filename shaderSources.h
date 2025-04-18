﻿#pragma once

namespace ShaderSources
{
    const char* vs1 = R"(
    #version 420 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    layout (location = 3) in vec3 aTangent;
    layout (location = 4) in vec3 aBitangent;
        
    //SHARED UNIFORMS ------------------------------------------------------------------------------------
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 dirLightSpaceMatrix;

    //OUT VARIABLES--------------------------------------------------------------------------------
    out vec2 TexCoords;
    out vec3 FragPos; //Frag pos in world position
    out vec3 Normal;
    out mat3 TBN; //TangentSpace -> WorldSpace
    out vec4 FragPosClipSpace;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);

        TexCoords = aTexCoords;
        FragPos = vec3(model * vec4(aPos, 1.0));

        mat3 normalMatrix = transpose(inverse(mat3(model)));
        vec3 N = normalize(normalMatrix * aNormal);
        vec3 T = normalize(normalMatrix * aTangent);
        vec3 B = cross(N,T);
        TBN = mat3(T,B,N);

        Normal = N;

        FragPosClipSpace = gl_Position;
    }
    )";

    const char* fs1 = R"(
    #version 420 core
    layout (location = 0) out vec4 FragColor;   //COLOR_ATTACHMENT_0

    struct PointLight {    
        vec3 position;   
        vec3 color; 
        float intensity;
        float range;
        bool isActive;
        bool castShadows;
    };  
    vec3 CalcPointLight(PointLight light, vec3 fragPos, vec3 viewDir, 
                vec3 diffuseColor, vec3 normal, vec3 baseSpecular, int index);
 
    struct DirLight {
        vec3 direction;
        vec3 color;
        float intensity;
        bool isActive;    
        bool castShadows;
    };
    vec3 CalcDirLight(DirLight light, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, vec3 normal, vec3 baseSpecular);
    
    float CascadeShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir);
    float PointShadowCalculation(vec3 fragPos, vec3 lightPos, int index);

    struct Material
    {
        sampler2D diffuse; //falls back to baseColor. Texture 0
        vec3 baseColor; 

        sampler2D normal; //may or may not exist. falls back to vertex data normals. Texture 1

        sampler2D specular; //may or may not exist. falls back to baseSpecular Texture 2
        float baseSpecular; //[0-1] hard scalar for specular
    };
        
    //IN VARIABLES--------------------------------------------------------------------------------
    in vec2 TexCoords;
    in vec3 Normal;
    in vec3 FragPos;
    in mat3 TBN;
    in vec4 FragPosClipSpace;
        
    //SAMPLERS------------------------------------------------------------------------------------
    //material.diffuse;                              //0
    //material.normal;                               //1
    //material.specular;                             //2
    uniform samplerCubeArray pointShadowMapArray;    //3
    uniform sampler2DArray cascadeShadowMaps;        //4
    //FREE
    //FREE
    //FREE
    //FREE
    //TERRAIN TEXTURE                                //9
    uniform sampler2D ssao;                          //10
    
    //SHARED UNIFORMS---------------------------------------------------------------------
    //light
    uniform PointLight pointLights[4];
    uniform int numPointLights;
    uniform DirLight dirLight;
    uniform float sceneAmbient;                     //updated every frame in SendOtherUniforms()
    //shadow
    uniform float cascadeDistances[4];              //set once in constructor
    uniform int cascadeCount;                       //set once in constructor
    uniform mat4 cascadeLightSpaceMatrices[5];      //updated every frame in RenderCascadedShadowMap()
    uniform float pointShadowProjFarPlane;          //for point shadow calculation
    //camera
    uniform float farPlane;                         //updated every frame in SendCameraUniforms() (unnessesarily)
    uniform float nearPlane;                        //updated every frame in SendCameraUniforms() (unnessesarily
    uniform vec3 viewPos;                           //updated every frame in SendCameraUniforms() 
    uniform mat4 view;                              //updated every frame in SendCameraUniforms() 
    //fog
    uniform int fogType;                            //SendOtherUniforms()
    uniform vec3 fogColor;             //updated every frame in SendOtherUniforms(). ({0,0,0} means fog is disabled)
    uniform float expFogDensity;                    //SendOTherUniforms()
    uniform float linearFogStart;                   //SendOtherUniforms()

    //UNIQUE UNIFORMS --------------------------------------------------------------------
    //material properties (Set by SendMaterialUniforms())
    uniform Material material;
    uniform bool usingDiffuseMap;
    uniform bool usingNormalMap;
    uniform bool usingSpecularMap;

    vec3 ChooseDiffuse();
    vec3 ChooseNormal();
    vec3 ChooseSpecular();
    float CalculateFogFactor();
    float CalculateLinearFog();

    const float gamma = 2.2;

    void main()
    {
        vec3 color = vec3(0.0f);
        vec3 viewDir = normalize(viewPos - FragPos);
    
        //Find which textures to use
        vec3 diffuse = ChooseDiffuse();
        vec3 normal = ChooseNormal();
        vec3 specular = ChooseSpecular();

        //LIGHTS
        for (int i = 0; i < numPointLights; i++)
        {
            color += CalcPointLight(pointLights[i], FragPos, viewDir, diffuse, normal, specular, i);
        }

        color += CalcDirLight(dirLight, FragPos, viewDir, diffuse, normal, specular);
        
        vec3 fragPosNDC = FragPosClipSpace.xyz / FragPosClipSpace.w; //perpective divide [-1,1]
        fragPosNDC = fragPosNDC * 0.5f + 0.5f; //[0,1]
        vec2 ssaoUV = fragPosNDC.xy;
        float occlusion = texture(ssao, ssaoUV).r;

        //Ambient lighting
        vec3 sceneAmbient = vec3(sceneAmbient) * diffuse * occlusion; 
        color += sceneAmbient;
        
        //atmospheric fog
        if (fogColor != vec3(0.0f))
        {
            float fogFactor = CalculateFogFactor();
            color = mix(fogColor, color, fogFactor);
        }

        FragColor = vec4(color, 1.0f);
           
        /*
        float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
        if (brightness > bloomThreshold) BrightColor = vec4(color, 1.0f);
        else BrightColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        */
    }

    const vec3 sampleOffsetDirections[20] = vec3[]
    (
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
        vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
        vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );  

    float CascadeShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir)
    {
        vec4 fragPosView = view * vec4(fragPos, 1.0f);
        float depth = abs(fragPosView.z);
            
        int layer = -1;
        for (int i = 0; i < cascadeCount; i++)
        {
            if (depth < cascadeDistances[i])
            {
                layer = i;
                break;
            }
        }
        if (layer == -1) layer = cascadeCount;
            
        vec4 fragPosLightSpace = cascadeLightSpaceMatrices[layer] * vec4(fragPos, 1.0f);

        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; //bring to [-1,1] (if in frustum)
        projCoords = projCoords * 0.5f + 0.5f; //bring to [0,1]
                
        float fragDepth = projCoords.z;
        if (fragDepth > 1.0f) return 0.0f;
            
        //BIAS (0.00035 is optimal NOTTT, no culling)
        float bias = max(0.00085 * (1.0 - dot(normal, lightDir)), 0.000085); //more bias with more angle.
        //if (layer == cascadeCount) bias *= 1 / (farPlane * 0.5f);
        //else bias *= 1 / (cascadeDistances[layer] * 0.5f);

        float shadow = 0.0f;

        //PCF---
        vec2 texelSize = 1.0f / vec2(textureSize(cascadeShadowMaps, 0));
    
        //This is like a convolution matrix.
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(cascadeShadowMaps, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
                shadow += fragDepth - bias > pcfDepth ? 1.0 : 0.0;  
            }    
        }
        shadow /= 9.0;
    
        //shadow = fragDepth - bias > closestDepth ? 1.0f : 0.0f;

        return shadow;  
    }

    float PointShadowCalculation(vec3 fragPos, vec3 lightPos, vec3 normal, int index)
    {
        vec3 lightToFrag = fragPos - lightPos;

        float fragDepth = length(lightToFrag); // [0, pointShadowProjFarPlane]
        float closestDepth = texture(pointShadowMapArray, vec4(lightToFrag, index)).r;// [0,1]
        closestDepth *= pointShadowProjFarPlane; //[0,1] -> [0, pointShadowProjFarPlane]
        
        float shadow = 0.0f;
        float bias = max(0.08 * (1.0 - dot(normalize(normal), normalize(lightToFrag))), 0.005);  
                
        //PCF---
        int samples = 20;
        float viewDistance = length(viewPos - fragPos);
        float diskRadius = (1.0 + (viewDistance / pointShadowProjFarPlane)) / 25.0; 
    
        for(int i = 0; i < samples; ++i)
        {
            float closestDepth = texture(pointShadowMapArray, vec4(lightToFrag + sampleOffsetDirections[i] * diskRadius, index)).r;
            closestDepth *= pointShadowProjFarPlane;   // move to [0, pointShadowProjFarPlane]

            if (fragDepth - bias > closestDepth) shadow += 1.0;
            
        }

        shadow /= float(samples);  

        //if (fragDepth - bias > closestDepth) shadow += 1.0f;

        return shadow;
    }

    vec3 CalcPointLight(PointLight light, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, vec3 normal, vec3 baseSpecular, int index)
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
        float maxDistance = (farPlane / 7.5f) * (light.range / 100.0f); //0: no light, 100: max reach
        if (maxDistance <= 0.0f) return vec3(0.0f);
        
        float t = clamp(1.0f - (distance / maxDistance), 0.0f, 1.0f);
        float attenuation = t * t; //quadratic attenuation
        // combine results
        vec3 diffuse  = light.intensity * light.color  * diff * diffuseColor;
        vec3 specular = light.intensity * light.color  * spec * baseSpecular;

        diffuse  *= attenuation;
        specular *= attenuation;
        
        if (!light.castShadows)
        {
            return (diffuse + specular);
        }
        else
        {
            float shadow = PointShadowCalculation(fragPos, light.position, normal, index);
            return (1.0f - shadow) * (diffuse + specular);
        }
    } 

    vec3 CalcDirLight(DirLight light, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, vec3 normal, vec3 baseSpecular)
    {
        //return normal;
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
        spec = pow(max(dot(halfwayDir, normal), 0.0), 64.0f);

        // combine results
        vec3 diffuse  = light.intensity * light.color  * diff * diffuseColor;
        vec3 specular = light.intensity * light.color  * spec * baseSpecular;
        
        if (!light.castShadows)
        {
            return (diffuse + specular);
        }
        else
        {
            //float shadow = DirShadowCalculation(FragPosDirLightSpace, normal, -direction);
            float shadow = CascadeShadowCalculation(FragPos, normal, -direction);
            return (1.0 - shadow) * (diffuse + specular);
        }
    }

    //Helper functions 
    vec3 ChooseDiffuse()
    {

        if (usingDiffuseMap)
        {
            //convert to linear space
            return pow(texture(material.diffuse, TexCoords).rgb, vec3(gamma));
        }
        else
        {
            return material.baseColor;
        }
    
    }

    vec3 ChooseNormal()
    {
        if (usingNormalMap)
        {
            vec3 normal = texture(material.normal, TexCoords).rgb;
            normal = normal * 2.0f - 1.0f; //[0,1] -> [-1, 1]
            normal = normalize(TBN * normal); //Tangent -> World (tbn is constucted with model matrix)
            return normal;
        }
        else
        {
            return normalize(Normal);
        }  
    }

    vec3 ChooseSpecular()
    {
        if (usingSpecularMap)
        {
            return vec3(texture(material.specular, TexCoords).r);
        }
        else
        {
            return vec3(material.baseSpecular);
        }   
    }
    
    float CalculateLinearFog()
    {
        float fogStart = nearPlane + linearFogStart;
        float fogEnd = farPlane;

        float cameraToFragmentDist = length(viewPos - FragPos);
        float fogRange = fogEnd - fogStart;
        float fogDist = fogEnd - cameraToFragmentDist;
        float fogFactor = clamp(fogDist / fogRange, 0.0f, 1.0f);
        return fogFactor;
    }
        
    float CalculateExponentialFog()
    {
        float cameraToFragDist = length(viewPos - FragPos);
        // *4 since we want around 0 when frag is at max distance
        // this is for average desntiy values like ~0.5
        float distRatio = (4.0f * cameraToFragDist) / farPlane;
        return exp(-distRatio * expFogDensity);
    }

    float CalculateExponentialSquaredFog()
    {
        float cameraToFragDist = length(viewPos - FragPos);
        // *4 since we want around 0 when frag is at max distance
        // this is for average desntiy values like ~0.5
        float distRatio = (4.0f * cameraToFragDist) / farPlane;
        
        return exp(-distRatio * expFogDensity * distRatio * expFogDensity);
    }
    
    float CalculateFogFactor()
    {
        float fogFactor;    
        
        switch (fogType)
        {
            //linear
            case 0:
                fogFactor = CalculateLinearFog();
                break;
            
            //exp
            case 1:
                fogFactor = CalculateExponentialFog();
                break;
            
            //sqr exp
            case 2:
                fogFactor = CalculateExponentialSquaredFog();
                break;
        }
        
        return fogFactor;
    }

    )";

    const char* fsLight = R"(
    #version 420 core
    layout (location = 0) out vec4 FragColor; //COLOR_ATTACHEMNT0
        
    uniform vec3 lightColor;
    uniform float intensity;

    void main()
    {
        FragColor = vec4(intensity * lightColor, 1.0); // a cube, that doesnt acutally emit light remember its purely a visual
            
        /*
        float brightness = dot(FragColor.rgb, vec3(0.2126f, 0.7152f, 0.0722f)); //Luminance formula, based on human vision
        if (brightness > bloomThreshold) BrightColor = vec4(lightColor, 1.0f);
        else BrightColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        */
    }
    )";

    const char* vsScreenQuad = R"(
    #version 420 core
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
    #version 420 core
    out vec4 FragColor;
  
    in vec2 TexCoords;
    
    uniform sampler2D hdrBuffer;    //0
    uniform sampler2D blurBuffer;   //1
    uniform float exposure;

    vec3 reinhardMap(vec3 hdrCol)
    {
        return hdrCol / (hdrCol + vec3(1.0f));
    }

    vec3 exposureMap(vec3 hdrCol, float exposure)
    {
        return vec3(1.0) - exp(-hdrCol * exposure);
    }
        
    vec3 cinematicMap(vec3 hdrCol, float exposure)
    {
        vec3 x = hdrCol * exposure;
    
        // Filmic curve parameters (Uncharted 2 style)
        const float A = 0.15;
        const float B = 0.50;
        const float C = 0.10;
        const float D = 0.20;
        const float E = 0.02;
        const float F = 0.30;
    
        vec3 toneMapped = ((x*(A*x + C*B) + D*E) / (x*(A*x + B) + D*F)) - E/F;
    
        // Normalize the tone-mapped result by a white point
        float white = 11.2;
        float whiteScale = ((white*(A*white + C*B) + D*E) / (white*(A*white + B) + D*F)) - E/F;
        toneMapped /= whiteScale;
    
        return toneMapped;
    }
            
        
    void main()
    {
        const float gamma = 2.2;
            
        vec3 hdrCol = texture(hdrBuffer, TexCoords).rgb;  
        vec3 blurCol = texture(blurBuffer, TexCoords).rgb;
        hdrCol += blurCol;
        //vec3 mapped = reinhardMap(hdrCol);
        vec3 mapped = exposureMap(hdrCol, exposure);           
        //vec3 mapped = cinematicMap(hdrCol, exposure);  
        mapped = pow(mapped, vec3(1.0 / gamma)); //gamma correction
        FragColor = vec4(mapped, 1.0f);
    }
    )";

    const char* fsBright = R"(
    #version 420 core
    out vec4 FragColor;
        
    in vec2 TexCoords;
        
    uniform sampler2D hdrScene; //0
    uniform float bloomThreshold;

    void main()
    {
        vec3 scene = texture(hdrScene, TexCoords).rgb;
        float luminance = dot(scene, vec3(0.2126, 0.7152, 0.0722));   
        
        vec3 result = luminance > bloomThreshold ? scene : vec3(0.0f);    
        FragColor = vec4(result, 1.0f);
    }
 
    )";

    const char* fsBlur = R"(
    #version 420 core
    out vec4 FragColor;
        
    in vec2 TexCoords;
        
    uniform sampler2D brightScene;
    uniform bool horizontal;

    float weights[5] = float[] (0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f); //gaussian
        
    void main()
    {
        vec2 texOffset = 1.0f / textureSize(brightScene, 0); //size of one texel
        vec3 result = vec3(0.0f);
            
        //Contribution of the current fragment
        result += texture(brightScene, TexCoords).rgb * weights[0];
            
        if (horizontal)
        {
            for (int i = 1; i < 5; i++)
            {
                vec2 currentSampleOffset = vec2(texOffset.x * i, 0.0f);
                result += texture(brightScene, TexCoords + currentSampleOffset).rgb * weights[i];
                result += texture(brightScene, TexCoords - currentSampleOffset).rgb * weights[i];
            }
        }
        else
        {
            for (int i = 1; i < 5; i++)
            {
                vec2 currentSampleOffset = vec2(0.0f, texOffset.y * i);
                result += texture(brightScene, TexCoords + currentSampleOffset).rgb * weights[i];
                result += texture(brightScene, TexCoords - currentSampleOffset).rgb * weights[i];
            }
        }

        FragColor = vec4(result, 1.0f);
    }
 
    )";

    const char* vsDirShadow = R"(
    #version 420 core

    layout (location = 0) in vec3 aPos;

    uniform mat4 lightSpaceMatrix; //view and projection combined
    uniform mat4 model;

    //This vertex shader simply converts a fragment to light space. Nothing else
    void main()
    {
        gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    }  
    )";

    const char* fsDirShadow = R"(
    #version 420 core

    void main()
    {
	    gl_FragDepth = gl_FragCoord.z; //set depth buffer manually if we want (this is done auto though so who cares...)
    }
    )";

    const char* vsCascadedShadow = R"(
    #version 420 core

    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 lightSpaceMatrix;

    //This vertex shader simply converts a fragment to light space. Nothing else
    void main()
    {
        gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    }  
    )";

    const char* fsCascadedShadow = R"(
    #version 420 core

    void main()
    {
	    // gl_FragDepth = gl_FragCoord.z; //set depth buffer manually if we want (this is done auto though so who cares...)
    }
    )";

    const char* gsCascadedShadow = R"(
    #version 420 core

    layout(triangles, invocations = 5) in;
    layout(triangle_strip, max_vertices = 3) out;
    
    uniform mat4 lightSpaceMatrices[5];

    void main()
    {          
        for (int i = 0; i < 3; ++i)
        {
            gl_Position = 
                lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
            gl_Layer = gl_InvocationID;
            EmitVertex();
        }
        EndPrimitive();
    } 
    )";

    const char* vsPointShadow = R"(
    #version 420 core

    layout (location = 0) in vec3 aPos;

    uniform mat4 lightSpaceMatrix; //view and projection combined
    uniform mat4 model;

    out vec3 FragPos;

    void main()
    {
        gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
        FragPos = vec3(model * vec4(aPos, 1.0));
    }  
    )";

    const char* fsPointShadow = R"(
    #version 420 core

    in vec3 FragPos;

    uniform vec3 lightPos;
    uniform float pointShadowProjFarPlane;

    void main()
    {
        float lightDistance = length(FragPos.xyz - lightPos);
    
        // map to [0,1] range by dividing by far_plane
        lightDistance = lightDistance / pointShadowProjFarPlane;
    
        // write this as modified depth
        gl_FragDepth = lightDistance;
    }
    )";

    const char* vsSkybox = R"(
    #version 420 core
    
    layout (location = 0) in vec3 aPos;
    
    uniform mat4 view;
    uniform mat4 projection;
    
    out vec3 TexCoords;
    
    void main()
    {
        //model is mat4(1.0) skybox size/pos doesnt change;
        //make z = w so z is always last in depth buffer. (w/w = 1.0 after perspective divide)
        gl_Position = (projection * view * vec4(aPos, 1.0)).xyww;
        TexCoords = aPos; //cubemap direction
    }  
    )";

        
    const char* fsSkybox = R"(
    #version 420 core
    layout (location = 0) out vec4 FragColor;

    in vec3 TexCoords;

    uniform samplerCube skyboxTexture;

    void main()
    {
	    FragColor = texture(skyboxTexture, TexCoords);
    }
    )";

    const char* vsTerrain = R"(
    #version 420 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords_VS_OUT;

    void main()
    {
        TexCoords_VS_OUT = aTexCoords;
	    gl_Position = vec4(aPos, 1.0f);
    }
    )";

    //tesselation control
    const char* tcsTerrain = R"(
    #version 420 core
    layout (vertices = 4) out;

    in vec2 TexCoords_VS_OUT[]; //since were working in patches
    out vec2 TexCoords_TCS_OUT[]; //likewise
            
    const int MIN_TESS_LEVEL = 16;
    const int MAX_TESS_LEVEL = 64;
    const float MIN_DISTANCE = 15.0f;
    const float MAX_DISTANCE = 200.0f;

    uniform mat4 model;
    uniform mat4 view;

    void main()
    {
        //NOTE: gl_in[] is a built-in, read-only array that contains all the vertices of the current patch. 
        //      (the data includes position and other per-vertex attributes)

        //NOTE:  gl_in[0] : TL WORLD
        //       gl_in[1] : TR WORLD
        //       gl_in[2] : BL WORLD
        //       gl_in[3] : BR WORLD
        //           Since we defined them in this order in vertex buffer creation. (See cpp code)

        //pass through
        gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
        TexCoords_TCS_OUT[gl_InvocationID] = TexCoords_VS_OUT[gl_InvocationID];
        //----

        //0 controls tessellation level
        if (gl_InvocationID == 0)
        {
            //TL WORLD
            vec4 viewSpacePos00 = view * model * gl_in[0].gl_Position; //see note but this is the position of a vertex of the current patch
            //TR WORLD
            vec4 viewSpacePos01 = view * model * gl_in[1].gl_Position;
            //BL WORLD
            vec4 viewSpacePos10 = view * model * gl_in[2].gl_Position;
            //BR WORLD
            vec4 viewSpacePos11 = view * model * gl_in[3].gl_Position;
                
            //note: distance from the camera [0,1]
            //TL WORLD
            float distance00 = clamp((abs(viewSpacePos00.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);
            //TR WORLD
            float distance01 = clamp((abs(viewSpacePos01.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);
            //BL WORLD
            float distance10 = clamp((abs(viewSpacePos10.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);
            //BR WORLD
            float distance11 = clamp((abs(viewSpacePos11.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);

            //interpolate tesselation based on closer vertex-------------------------------------------------
                
            //NOTE: OL-0  is left side of cube. So take the min of the distances of the TL and BL vertices.
            //NOTE: refer to diagram on openGL wiki, but with 0,0 at the top left.
            //       why? Try mentally overlaying the origina diagram on flat terrain. The coords dont match.
            //       +y is tess space is like -z in world space. This is why top and bottom are swapped.
            float tessLevel0 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance10));
            //Ol-1: top. TL and TR
            float tessLevel1 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01));
            //Ol-2: right. BR and TR
            float tessLevel2 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11));
            //Ol-3: bottom. BL and BR
            float tessLevel3 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance11, distance10));

            gl_TessLevelOuter[0] = tessLevel0;  //left
            gl_TessLevelOuter[1] = tessLevel1;  //top  'bot' in local tessCoords
            gl_TessLevelOuter[2] = tessLevel2;  //right
            gl_TessLevelOuter[3] = tessLevel3;  //bot  'top' in local tessCoords
                
            gl_TessLevelInner[0] = max(tessLevel1, tessLevel3); //inner top/bot
            gl_TessLevelInner[1] = max(tessLevel0, tessLevel2); //inner l/r
        }
    }
    )";

    //tesselation eval, for each tesselated vertex
    const char* tesTerrain = R"(
    #version 420 core
    layout (quads, fractional_odd_spacing, cw) in;

    uniform sampler2D heightMap;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    const float heightScale = 64.0f;
    const float heightOffset = -16.0f;

    in vec2 TexCoords_TCS_OUT[]; //array of texture coords of the current patch where this current vertex is located.
                        //since its quads, one in each corner. (Control points)
        
    out vec3 Normal;
    out vec2 TexCoords;
    out vec3 FragPos;
    out float Height;
    out mat3 TBN;
    out vec4 FragPosClipSpace;

    const vec2 worldTexelSize = vec2(1.0f, 1.0f);
    //For simplicity, texturesize is equivalnt to world size.

    void main()
    {
        //get tess coords [0,1]
        float u = gl_TessCoord.x;
        float v = gl_TessCoord.y;

        //texcoord control points
        vec2 t00 = TexCoords_TCS_OUT[0];
        vec2 t01 = TexCoords_TCS_OUT[1];
        vec2 t10 = TexCoords_TCS_OUT[2];
        vec2 t11 = TexCoords_TCS_OUT[3];

        //bilinear interpolate the tessCoord texCoord from control points
        vec2 t0 = (t01 - t00) * u + t00;
        vec2 t1 = (t11 - t10) * u + t10;
        vec2 currentTexCoord = (t1 - t0) * v + t0; //the texcoord of the current vertex based of its control points.
            
        //the height of this vertex.
        float currentHeight = texture(heightMap, currentTexCoord).r * heightScale + heightOffset;
            
        //position control points (model space)
        vec4 p00 = gl_in[0].gl_Position; //TL 
        vec4 p01 = gl_in[1].gl_Position; //TR 
        vec4 p10 = gl_in[2].gl_Position; //BL 
        vec4 p11 = gl_in[3].gl_Position; //BR 

        //bilinear interpolate the vertexes position from control points
        vec4 p0 = (p01 - p00) * u + p00;
        vec4 p1 = (p11 - p10) * u + p10;
        vec4 position = (p1 - p0) * v + p0;
            
        //compute normals
        vec4 uVec = p01 - p00; //TL -> TR (1,0,0)
        vec4 vVec = p10 - p00; //TL -> BL (0,0,1)
        vec4 patchNormal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) ); //should be (0,1,0)

        //use normal to extrude plane vertex
        position += patchNormal * currentHeight;

        //NORMAL CALCLUATION - FINITE DIFFERENCES -------------------
        vec2 texelSize = 1.0f / textureSize(heightMap, 0);
        //Get texCoords for neighbors
        vec2 texCoord_L = currentTexCoord - vec2(texelSize.x, 0);
        vec2 texCoord_R = currentTexCoord + vec2(texelSize.x, 0);
        vec2 texCoord_B = currentTexCoord - vec2(0, texelSize.y);
        vec2 texCoord_T = currentTexCoord + vec2(0, texelSize.y);

        //Get heights of neighboring texels
        float height_L = texture(heightMap, texCoord_L).r * heightScale + heightOffset;
        float height_R = texture(heightMap, texCoord_R).r * heightScale + heightOffset;
        float height_B = texture(heightMap, texCoord_B).r * heightScale + heightOffset;
        float height_T = texture(heightMap, texCoord_T).r * heightScale + heightOffset;
 
        //Calculate tangent vectors
        //--
        //  This simulates moving along +U (x axis in model space). 
        //  going from L (texcoord - 1) to R (texcoord + 1) is total of 2.0 units along the x axis.
        //  Then the change is height (from L to R) is R - L.
        vec3 tangentU = vec3(2.0 * worldTexelSize.x, height_R - height_L, 0.0); //dU (partial with respect to U @ texCoord)

        //  This simulates moving along +V (-z axis in model space). 
        //  going from B (texcoord - 1) to T (texcoord + 1) is total of 2.0 units along the -z axis.
        //  Then the change is height (from B to T) is T - B.
        vec3 tangentV = vec3(0.0, height_T - height_B, -2.0 * worldTexelSize.y); //dV (partial with respect to V @ texCoord)

        //world space Normal/Tangent
        vec3 normalLocal = cross(tangentU, tangentV);

        mat3 normalMatrix = transpose(inverse(mat3(model)));
        vec3 worldNormal = normalize(normalMatrix * normalLocal);
    
        //calculate TBN matrix
        vec3 tangentLocal = normalize(tangentU - dot(tangentU, normalLocal) * normalLocal);

        vec3 worldTangent = normalize(normalMatrix * tangentLocal);
        vec3 worldBiTangent = normalize(cross(worldNormal, worldTangent));

        mat3 tbn = mat3(worldTangent,worldBiTangent,worldNormal);

        //FINAL OUT VALUES
        gl_Position = projection * view * model * position;
        FragPos = vec3(model * position);
        Normal = worldNormal;
        TexCoords = currentTexCoord;
        Height = currentHeight;
        TBN = tbn;
        FragPosClipSpace = gl_Position;
    }
    )";

    const char* fsTerrain = R"(
    #version 420 core

    in float Height;
    in vec3 Normal;

    out vec4 FragColor;

    void main()
    {
        float h = (Height + 16)/64.0f;
        //FragColor = vec4(h, h, h, 1.0);
        FragColor = vec4(Normal, 1.0);
    }             
    )";

    const char* vsGeometryPass = R"(
    #version 420 core

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    out vec3 FragPos;
    out vec3 Normal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
	    //Frag pos in view space
	    vec4 viewSpaceFragPos = view * model * vec4(aPos, 1.0f);
	    FragPos = viewSpaceFragPos.xyz; //view space

	    //View space normal matrix
	    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
	    Normal = normalMatrix * aNormal; //view space

	    gl_Position = projection * viewSpaceFragPos; //clip space
    }
    )";

    const char* fsGeometryPass = R"(
    #version 420 core
    layout (location = 0) out vec4 gNormal;   //viewspace
    layout (location = 1) out vec4 gPosition; //viewspace    


    in vec3 FragPos;
    in vec3 Normal;

    void main()
    {
        gNormal = vec4(normalize(Normal), 1.0f);
        gPosition = vec4(FragPos, 1.0f);
    }             
    )";

    const char* vsSSAO = R"(
    #version 420 core
    
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;
    
    out vec2 TexCoords;

    void main()
    {
        TexCoords = aTexCoords;
        gl_Position = vec4(aPos, 1.0); //workin on a quad
    }
            
    )";

    const char* fsSSAO = R"(
    #version 420 core
    
    in vec2 TexCoords;

    uniform sampler2D gNormal;              //0
    uniform sampler2D gPosition;            //1
    uniform sampler2D ssaoNoiseTexture;     //2

    uniform vec3 samples[32];            //hemisphere vectors in tangent space
    uniform mat4 projection;
    
    uniform float screenWidth;
    uniform float screenHeight;

    float radius = 0.5f;

    out float FragColor;

    void main()
    {
        //The amount to scale TexCoords (currently [0,1]) by for tiling.
        vec2 noiseScale = vec2(screenWidth / 4.0f, screenHeight / 4.0f);

        vec3 fragPosView = texture(gPosition, TexCoords).xyz; 
        vec3 normalView = normalize(texture(gNormal, TexCoords).rgb);
        //random vector with z = 0
        vec3 randomVec = normalize(texture(ssaoNoiseTexture, TexCoords * noiseScale).xyz);     

        vec3 tangent   = normalize(randomVec - normalView * dot(randomVec, normalView));
        vec3 bitangent = cross(normalView, tangent);

        //view-space TBN
        mat3 TBN       = mat3(tangent, bitangent, normalView);  //tangent -> view
        
        float occlusion = 0.0f;
        
        for (int i = 0; i < 32; i++)
        {
            vec3 samplePosView = TBN * samples[i]; //hemisphere vector in tangent -> view-space
            samplePosView = fragPosView + samplePosView * radius; //true sample position (og frag + hemisphere vector) IN VIEWSPACE

            //get samplePos to NDC to sample texture
            vec4 samplePosNDC = vec4(samplePosView, 1.0f);
            samplePosNDC = projection * samplePosNDC; //view -> clip
            samplePosNDC.xyz /= samplePosNDC.w;
            samplePosNDC.xyz  = samplePosNDC.xyz * 0.5 + 0.5; // [-1,1] -> [0,1]
            
            //get texture depth at samplePosNDC
            float gPositionSampleDepthView = texture(gPosition, samplePosNDC.xy).z;
            
            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPosView.z - gPositionSampleDepthView));
            float bias = 0.025f;
            //check if the 
            occlusion += (gPositionSampleDepthView >= samplePosView.z + bias ? 1.0 : 0.0) * rangeCheck;
        }   
        
        occlusion = 1.0f - (occlusion / 32); //so we can directly multiply fragment in light shader
        float power = 3.0f;
	    FragColor = pow(occlusion, power);
    }        
    )";



    const char* fsSSAOBlur = R"(
    #version 420 core
    
    out float FragColor;

    in vec2 TexCoords;
    
    uniform sampler2D ssaoTexture;

    void main()
    {
        vec2 texelSize = 1.0 / vec2(textureSize(ssaoTexture, 0));
        float result = 0.0f;

        for (int x = -2; x < 2; x++) //dont include +x and +y since noise texture is 4x4
        {
            for (int y = -2; y < 2; y++)
            {
                vec2 offset = vec2(float(x), float(y)) * texelSize;
                result += texture(ssaoTexture, TexCoords + offset).r;
            }
        }

        FragColor = result / (4.0f * 4.0f); //noise texture dimensions
    }
    )";
}