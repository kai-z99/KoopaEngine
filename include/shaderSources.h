﻿#pragma once

namespace ShaderSources
{
    const char* vs1 = R"(
    #version 450 core
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
    #version 450 core
    layout (location = 0) out vec4 FragColor;   //COLOR_ATTACHMENT_0

    struct GPUPointLight    
    {    
        vec4 positionRange;   
        vec4 colorIntensity; 
        uint isActive;
        int shadowMapIndex;
        uint pad0, pad1; //48
    };
        
    vec3 CalcPointLight(GPUPointLight light, vec3 fragPos, vec3 viewDir, 
                vec3 diffuseColor, vec3 normal, vec3 baseSpecular);

    vec3 CalcPointLightPBR(GPUPointLight light, vec3 fragPos, vec3 viewDir, 
                vec3 albedo, vec3 normal, float metallic, float roughness, float ao);
 
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

    struct PBRMaterial
    {
        sampler2D albedo;
        sampler2D normal;
        sampler2D metallic;
        sampler2D roughness;
        sampler2D ao;
        sampler2D height;
    };

    vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
    {
        return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    } 

    vec2 ParallaxOcclusionMap(vec2 texCoords, vec3 viewDir, sampler2D heightMap);


    //IN VARIABLES--------------------------------------------------------------------------------
    in vec2 TexCoords;
    in vec3 Normal;
    in vec3 FragPos;
    in mat3 TBN;
    in vec4 FragPosClipSpace;

    //SAMPLERS------------------------------------------------------------------------------------
    //material.diffuse; OR pbrmatieral.albedo        //0
    //material.normal; OR pbrmaterial.normal         //1
    //material.specular; OR prrmaterial.metallic     //2
    uniform samplerCubeArray pointShadowMapArray;    //3
    uniform sampler2DArray cascadeShadowMaps;        //4
    //pbrmaterial.roughness                          //5
    //pbrmaterial.ao                                 //6
    uniform samplerCube irradianceMap;               //7
    uniform samplerCube prefilterMap;                //8
    //TERRAIN TEXTURE                                //9
    uniform sampler2D ssao;                          //10
    uniform sampler2D brdfLUT;                       //11
    //pbrmaterial.height                             //12
    
    //SHARED UNIFORMS---------------------------------------------------------------------
    //light
    uniform bool usingPBR;
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
    //forward+
    uniform uint tileSize = 16u;
    uniform uvec2 screen = uvec2(1920, 1080);
    const uint MAX_LIGHTS_PER_TILE = 256u;

    //UNIQUE UNIFORMS --------------------------------------------------------------------
    //material properties (Set by SendMaterialUniforms())
    uniform Material material;
    uniform bool hasAlpha;
    uniform bool usingDiffuseMap;
    uniform bool usingNormalMap;
    uniform bool usingSpecularMap;
    uniform PBRMaterial PBRmaterial;

    uniform bool useSSS = false;
    uniform vec3 sssColor = vec3(0.02f, 0.42f, 0.02f);
    uniform float sigmaT = 1500.0f; //1500

    vec3 ChooseDiffuse();
    vec3 ChooseNormal();
    vec3 ChooseSpecular();
    float CalculateFogFactor();
    float CalculateLinearFog();

    const float gamma = 2.2;

    layout(std430, binding = 1) buffer Lights
    {
        GPUPointLight lights[];  
    };

    layout(std430, binding = 2) buffer TileIndices
    {
        uint indices[];
    };
    
    layout(std430, binding = 3) buffer TileCount
    {
        uint counts[];
    };

    )"
    R"(
    void main()
    {
        vec3 color = vec3(0.0f);
        vec3 viewDir = normalize(viewPos - FragPos);    
        
        //Find which textures to use for blinn phong
        vec3 diffuse = ChooseDiffuse();
        vec3 normal = ChooseNormal();
        vec3 specular = ChooseSpecular();

        uvec2 pix = uvec2(gl_FragCoord.xy);
        uvec2 tile = pix / tileSize;
        uint tileID = tile.y * uint(screen.x / 16) + tile.x;
    
        uint lightsInTile = counts[tileID];
        
        //LIGHTS
        if (!usingPBR)
        {
            for (uint i = 0u; i < lightsInTile; i++)
            {
                uint lightIndex = indices[tileID * MAX_LIGHTS_PER_TILE + i];

                GPUPointLight g = lights[lightIndex];

                color += CalcPointLight(g, FragPos, viewDir, diffuse, normal, specular);
            }

            color += CalcDirLight(dirLight, FragPos, viewDir, diffuse, normal, specular);

            vec3 fragPosNDC = FragPosClipSpace.xyz / FragPosClipSpace.w; //perpective divide [-1,1]
            fragPosNDC = fragPosNDC * 0.5f + 0.5f; //[0,1]
            vec2 ssaoUV = fragPosNDC.xy;
            float occlusion = texture(ssao, ssaoUV).r;

            //Ambient lighting
            vec3 sceneAmbient = vec3(sceneAmbient) * diffuse * occlusion; 
            color += sceneAmbient;
        }
        else
        {            
            vec2 offsetTexCoords = ParallaxOcclusionMap(TexCoords, normalize(transpose(TBN) * viewDir), PBRmaterial.height);
            vec3 albedo = pow(texture(PBRmaterial.albedo, offsetTexCoords).rgb, vec3(gamma)); //linear space
            vec3 N = texture(PBRmaterial.normal, offsetTexCoords).xyz;
            N = N * 2.0f - 1.0f; //[0,1] -> [-1, 1]
            N = normalize(TBN * N); //Tangent -> World (tbn is constucted with model matrix)
            float metallic = texture(PBRmaterial.metallic, offsetTexCoords).r;
            float roughness = texture(PBRmaterial.roughness, offsetTexCoords).r;
            //roughness = max(roughness, 0.04); //avoid sparkling
            float ao = texture(PBRmaterial.ao, offsetTexCoords).r;
            vec3 reflectionDir = reflect(-viewDir, N);
            
            //Calculate Lo = full integral for each point light (both spec and scatter components)
            for (uint i = 0u; i < lightsInTile; i++)
            {
                uint lightIndex = indices[tileID * MAX_LIGHTS_PER_TILE + i];

                GPUPointLight g = lights[lightIndex];

                color += CalcPointLightPBR(g, FragPos, viewDir, albedo, N, metallic, roughness, ao);
            }
            
            
            //IBL INDIRECT AMBIENT LIGHTING 
            vec3 F0 = vec3(0.04); 
            F0 = mix(F0, albedo, metallic);
            vec3 F = FresnelSchlickRoughness(max(dot(N, viewDir), 0.0), F0, roughness);
            vec3 kS = F;
            vec3 kD = 1.0 - kS;
            kD *= 1.0 - metallic;	

            //Calculate IBL diffuse part of integral
            vec3 irradiance = texture(irradianceMap, N).rgb; //integral part DIVIDED BY PI
            vec3 diffuseIntegral = kD * albedo * irradiance; //kd * flambert * integral //diffuse integral solved

            //Calculate IBL specular part of integral
            vec3 prefilterColor = textureLod(prefilterMap, reflectionDir, roughness * 4.0).rgb;
            vec2 brdf = texture(brdfLUT, vec2(max(dot(N, viewDir), 0), roughness)).rg;
            vec3 specularIntegral = prefilterColor * (F0 * brdf.x + brdf.y );

            float k = 1;
            vec3 ambient = (diffuseIntegral + specularIntegral) * ao; //put the integral together and times by an ao term...
            //vec3 ambient = (diffuseIntegral * k) * ao;
            color += ambient;
        }

        //atmospheric fog
        if (fogColor != vec3(0.0f))
        {
            float fogFactor = CalculateFogFactor();
            color = mix(fogColor, color, fogFactor);
        }

        if (hasAlpha && !usingPBR) FragColor = vec4(color, texture(material.diffuse, TexCoords).a);
        else FragColor = vec4(color, 1.0f);

        //FragColor = vec4( (ParallaxOcclusionMap(TexCoords, normalize(transpose(TBN) * viewDir), PBRmaterial.height) - TexCoords)*5.0 + 0.5, 0.0, 1.0 );
    }

    //SHADOWS
    )"
    R"(
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
        float bias = max(0.0012 * (1.0 - dot(normal, lightDir)), 0.00012); //more bias with more angle.
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
                shadow += fragDepth - bias > pcfDepth ? 0.0 : 1.0;  
            }    
        }
        shadow /= 9.0;
    
        //shadow = fragDepth - bias > closestDepth ? 1.0f : 0.0f;

        return shadow;  
    }

    float LightBleedReduction(float p_max, float amount)
    {
        //if p_max < amount, p_max = 0 (so fully shadows random small lighted places below "amount")
        //if p_max > 1, p_max = 1 (full light)
        return smoothstep(amount, 1, p_max);
    }

    float PointShadowCalculation(vec3 fragPos, vec3 lightPos, vec3 normal, int index)
    {
        vec3 lightToFrag = fragPos - lightPos;
        float fragDepth = length(lightToFrag); // [0, pointShadowProjFarPlane]

        float Ed = texture(pointShadowMapArray, vec4(normalize(lightToFrag), index)).r * pointShadowProjFarPlane; // E[d]
        float EdSq = texture(pointShadowMapArray, vec4(normalize(lightToFrag), index)).g * pointShadowProjFarPlane * pointShadowProjFarPlane; // E[d]^2
        
        if (fragDepth <= Ed) //definitaly lit
        {
            return 1.0f;
        }
            
        float variance = EdSq - (Ed * Ed);  //sigma squared
        variance = max(variance, 0.00001f);
        float d = fragDepth - Ed;      
            
        float pMax = variance / (variance + (d * d));

        float shadow = LightBleedReduction(pMax, 0.35f);
        return clamp(shadow, 0.0f, 1.0f); 
    }

    //BLINN PHONG
    )"
    R"(
    vec3 CalcPointLight(GPUPointLight light, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, vec3 normal, vec3 baseSpecular)
    {
        if (light.isActive == 0u)
        {
            return vec3(0.0f, 0.0f, 0.0f);
        }
        
        vec3 lightDir = normalize(light.positionRange.xyz - fragPos);

        // diffuse shading
        float diff = max(dot(normal, lightDir), 0.0);

        // specular shading
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec;
        spec = pow(max(dot(halfwayDir, normal), 0.0), 64.0f);
            
        // attenuation 
        float distance    = length(light.positionRange.xyz - fragPos);
        float maxDistance = light.positionRange.w;

        if (distance > maxDistance) return vec3(0.0f);
        
        float t = 1.0f - (distance / maxDistance);
        float attenuation = t * t; //quadratic attenuation
        // combine results
        vec3 diffuse  = light.colorIntensity.w * light.colorIntensity.rgb  * diff * diffuseColor;
        vec3 specular = light.colorIntensity.w * light.colorIntensity.rgb  * spec * baseSpecular;

        diffuse  *= attenuation;
        specular *= attenuation;
        
        if (light.shadowMapIndex == -1)
        {
            return (diffuse + specular);
        }
        else
        {
            float shadow = PointShadowCalculation(fragPos, light.positionRange.xyz, normal, light.shadowMapIndex);
            return shadow * (diffuse + specular);
        }
    } 

    vec3 calculateDirSSS(DirLight light, vec3 fragPos)
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
    
        vec4 lightSpacePos    = cascadeLightSpaceMatrices[layer] * vec4(fragPos, 1.0);
        vec3 projectedPos    = lightSpacePos.xyz / lightSpacePos.w;
        projectedPos = projectedPos * 0.5 + 0.5;      
        
        float nearCascade = (layer == 0)? nearPlane : cascadeDistances[layer - 1]; 
        float farCascade = cascadeDistances[layer];                                
       
        float entryDepth = texture(cascadeShadowMaps, vec3(projectedPos.xy, layer)).r; 
        float exitDepth = projectedPos.z; 

        float thickness = max(exitDepth - entryDepth - 0.001, 0);
        //thickness = abs(exitDepth - entryDepth - 0.005);
        //if (thickness < 0.0001) return vec3(0);

        //return vec3(thickness,0,0);
        float backlight = exp(-sigmaT * thickness); //beer-lambert

        //return vec3(backlight,0,0);

        return /*light.intensity * light.color *  */     backlight * sssColor;    
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
        vec3 diffuse;
        if (useSSS)
        {
            vec3 sss = vec3(0.0f);
            if (dot(direction, normal) < -0.1f)
            {
                sss = calculateDirSSS(light, fragPos);
            }   

            float wrap = 0.5f;
            float d = ((dot(direction, normal) + wrap) / (1 + wrap) );

            float diffWrap = max(d, 0.0f);
            float scatterWidth = 0.3f;
            float scatter = smoothstep(0.0, scatterWidth, d) *
                      smoothstep(scatterWidth * 2.0, scatterWidth, d); 
        
            
            diffuse =  light.intensity * light.color  * diffWrap * diffuseColor + (scatter * sssColor) + sss;
        }
        else
        {
            float diff = max(dot(direction, normal), 0.0f);
            diffuse  = light.intensity * light.color  * diff * diffuseColor;
        }

        //spec
        vec3 halfwayDir = normalize(direction + viewDir);
        float spec;
        spec = pow(max(dot(halfwayDir, normal), 0.0), 64.0f);
        vec3 specular = light.intensity * light.color  * spec * baseSpecular;

        // combine results
        if (!light.castShadows)
        {
            return (diffuse + specular);
        }
        else
        {
            //float shadow = DirShadowCalculation(FragPosDirLightSpace, normal, -direction);
            float shadow = CascadeShadowCalculation(FragPos, normal, -direction);
            return shadow * (diffuse + specular);
        }
    }

    vec2 ParallaxOcclusionMap(vec2 texCoords, vec3 viewDir, sampler2D depthMap)
    {
        const float heightScale = 0.07f;

        const float minLayers = 8.0f;
        const float maxLayers = 24.0f;

        //(0,0,1) is up in tangent space. More layers at lower angles
        float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0,0,1), viewDir)));
        
        float layerDepth = 1.0f / numLayers;

        //max shift amount, based on cos(theta) = viewDir.z
        vec2 P = viewDir.xy / max(viewDir.z, 0.1) * heightScale;
        //note this point toward the viewDir since it comes from viewDir projected

        vec2 deltaTexCoords = P / numLayers;
        vec2 currentTexCoords = texCoords;
        float currentDepthMapValue = 1 - texture(depthMap, currentTexCoords).r;

        float currentLayerDepth = 0.0f;
        while (currentLayerDepth < currentDepthMapValue) //curr is "above" the depth
        {
            //push away is subtract since P pointed towards view
            currentTexCoords -= deltaTexCoords;
            currentDepthMapValue = 1 - texture(depthMap, currentTexCoords).r;
            
            currentLayerDepth += layerDepth;
        }

        //pull once towards cam
        vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
        float prevDepthMapValue = 1 - texture(depthMap, prevTexCoords).r;
        float prevLayerDepth = currentLayerDepth - layerDepth;        

        //get depth error: height - marchDepth
        float depthDeltaCurr = currentDepthMapValue - currentLayerDepth;    
        float depthDeltaPrev = prevDepthMapValue - prevLayerDepth;
        
        //lerp bewteen the before collision depth and after collision depth
        float weight = depthDeltaCurr / (depthDeltaCurr - depthDeltaPrev);
        vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

        return finalTexCoords;
    }

    //PBR FUNCTIONS
    )"
    R"(

    float DistributionGGX(vec3 N, vec3 H, float roughness)
    {
        const float PI = 3.14159265359;

        float a      = roughness*roughness;
        float a2     = a*a;
        float NdotH  = max(dot(N, H), 0.0);
        float NdotH2 = NdotH*NdotH;
	
        float num   = a2;
        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
        denom = PI * denom * denom;
	
        return num / denom;
    }

    float GeometrySchlickGGX(float NdotV, float roughness)
    {
        float r = (roughness + 1.0);
        float k = (r*r) / 8.0;

        float num   = NdotV;
        float denom = NdotV * (1.0 - k) + k;
	
        return num / denom;
    }

    float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
    {
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        float ggx2  = GeometrySchlickGGX(NdotV, roughness); //geometry obstruction
        float ggx1  = GeometrySchlickGGX(NdotL, roughness); //geometry shadowing
	
        return ggx1 * ggx2;
    }

    vec3 FresnelSchlick(float cosTheta, vec3 F0)
    {
        return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    }

    vec3 CalcPointLightPBR(GPUPointLight light, vec3 fragPos, vec3 viewDir, vec3 albedo, vec3 normal, float metallic, float roughness, float ao)
    {
        const float PI = 3.14159265359;

        //0.04 is acceptable for most materials
        vec3 F0 = vec3(0.04);
        F0 = mix(F0, albedo, metallic); //albedo stores scatter color of dialectrics, base reflectivity metals. (2 pipelines)

        vec3 Lo = vec3(0.0);
        vec3 lightDir = normalize(light.positionRange.xyz - fragPos);
        vec3 halfway = normalize(viewDir + lightDir);

        float distance = length(light.positionRange.xyz - fragPos);
        float maxDistance = light.positionRange.w;
        if (distance > maxDistance) return vec3(0.0f);

        //falloff based on UE4's implementation
        float t = clamp(1.0f - pow(distance / maxDistance, 4), 0, 1);
        float falloffNum = t * t;
        float d2 = distance * distance;
        float falloffDenom = d2 + 1;
        float attenuation = falloffNum / falloffDenom;

        vec3 radiance = light.colorIntensity.rgb * light.colorIntensity.w * attenuation;
        vec3 fLambert = albedo / PI;

        //cook-torrence BRDF
        float NDF = DistributionGGX(normal, halfway, roughness);
        float G = GeometrySmith(normal, viewDir, lightDir, roughness);
        vec3 F = FresnelSchlick(max(dot(halfway, viewDir), 0.0f), F0);
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(viewDir, normal), 0.0) * max(dot(lightDir, normal), 0.0) + 0.00001;
        vec3 fCookTorrence = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= 1.0f - metallic; //kd theoretically either exists or is 0. (binary metal values)

        float NdotL = max(dot(normal, lightDir),0.0f);
        Lo += (kD * fLambert + fCookTorrence) * radiance * NdotL;
        
        return Lo;
    }

    //HELPERS
    )"
    R"(
    
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

    
    const char* vsCube = R"(
    #version 450 core
    layout (location = 0) in vec3 aPos;
    
    out vec3 WorldPos;

    uniform mat4 projection;
    uniform mat4 view;
    
    void main()
    {
        WorldPos = aPos;
        gl_Position = projection * view * vec4(aPos, 1.0f);
    }    


    )";

    const char* fsEquirectangularToCubemap = R"(
    #version 450 core
    out vec4 FragColor;
    in vec3 WorldPos;

    uniform sampler2D equirectangularMap;

    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 SampleSphericalMap(vec3 v)
    {
        vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
        uv *= invAtan;
        uv += 0.5;
        return uv;
    }

    void main()
    {		
        vec2 uv = SampleSphericalMap(normalize(WorldPos));
        vec3 color = texture(equirectangularMap, uv).rgb;
    
        FragColor = vec4(color, 1.0);
    }
    )";

    const char* fsIrradiance = R"(
    #version 450 core
    out vec4 FragColor;
    in vec3 WorldPos;

    uniform samplerCube environmentMap;

    const float PI = 3.14159265359;

    void main()
    {		
        vec3 N = normalize(WorldPos);

        vec3 irradiance = vec3(0.0);   
    
        // tangent space calculation from origin point
        vec3 up    = vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(up, N));
        up         = normalize(cross(N, right));
       
        float sampleDelta = 0.025;
        float nrSamples = 0.0;
        for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
        {
            for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
            {
                // spherical to cartesian (in tangent space)
                vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
                // tangent space to world
                vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

                irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
                nrSamples++;
            }
        }
        irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
        FragColor = vec4(irradiance, 1.0);
    }
    )";

    //uses cube vs
    const char* fsPreFilter = R"(
    #version 450 core
    out vec4 FragColor;
    in vec3 WorldPos;

    uniform samplerCube environmentMap;
    uniform float roughness;

    const float PI = 3.14159265359;
    // ----------------------------------------------------------------------------
    float DistributionGGX(vec3 N, vec3 H, float roughness)
    {
        float a = roughness*roughness;
        float a2 = a*a;
        float NdotH = max(dot(N, H), 0.0);
        float NdotH2 = NdotH*NdotH;

        float nom   = a2;
        float denom = (NdotH2 * (a2 - 1.0) + 1.0);
        denom = PI * denom * denom;

        return nom / denom;
    }
    // ----------------------------------------------------------------------------
    // http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    // efficient VanDerCorpus calculation.
    float RadicalInverse_VdC(uint bits) 
    {
         bits = (bits << 16u) | (bits >> 16u);
         bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
         bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
         bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
         bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
         return float(bits) * 2.3283064365386963e-10; // / 0x100000000
    }
    // ----------------------------------------------------------------------------
    vec2 Hammersley(uint i, uint N)
    {
	    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
    }
    // ----------------------------------------------------------------------------
    vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
    {
	    float a = roughness*roughness;
	
	    float phi = 2.0 * PI * Xi.x;
	    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	    // from spherical coordinates to cartesian coordinates - halfway vector
	    vec3 H;
	    H.x = cos(phi) * sinTheta;
	    H.y = sin(phi) * sinTheta;
	    H.z = cosTheta;
	
	    // from tangent-space H vector to world-space sample vector
	    vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	    vec3 tangent   = normalize(cross(up, N));
	    vec3 bitangent = cross(N, tangent);
	
	    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	    return normalize(sampleVec);
    }
    // ----------------------------------------------------------------------------
    void main()
    {		
        vec3 N = normalize(WorldPos);
    
        // make the simplifying assumption that V equals R equals the normal 
        vec3 R = N;
        vec3 V = R;

        const uint SAMPLE_COUNT = 1024u;
        vec3 prefilteredColor = vec3(0.0);
        float totalWeight = 0.0;
    
        for(uint i = 0u; i < SAMPLE_COUNT; ++i)
        {
            // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
            vec2 Xi = Hammersley(i, SAMPLE_COUNT);
            vec3 H = ImportanceSampleGGX(Xi, N, roughness);
            vec3 L  = normalize(2.0 * dot(V, H) * H - V); //heres the sample

            float NdotL = max(dot(N, L), 0.0);
            if(NdotL > 0.0)
            {
                // sample from the environment's mip level based on roughness/pdf to reduce spots
                float D   = DistributionGGX(N, H, roughness);
                float NdotH = max(dot(N, H), 0.0);
                float HdotV = max(dot(H, V), 0.0);
                float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 

                float resolution = 512.0; // resolution of source cubemap (per face)
                float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
                float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
                
                float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
            
                prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
                totalWeight      += NdotL;
            }
        }

        prefilteredColor = prefilteredColor / totalWeight;

        FragColor = vec4(prefilteredColor, 1.0);
    }
    
    )";

    const char* vsBRDF = R"(
    #version 450 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;

    out vec2 TexCoords;

    void main()
    {
        TexCoords = aTexCoords;
	    gl_Position = vec4(aPos, 1.0);
    }

    )";

    const char* fsBRDF = R"(
    #version 450 core
    out vec2 FragColor;
    in vec2 TexCoords;

    const float PI = 3.14159265359;
    // ----------------------------------------------------------------------------
    // http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    // efficient VanDerCorpus calculation.
    float RadicalInverse_VdC(uint bits) 
    {
         bits = (bits << 16u) | (bits >> 16u);
         bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
         bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
         bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
         bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
         return float(bits) * 2.3283064365386963e-10; // / 0x100000000
    }
    // ----------------------------------------------------------------------------
    vec2 Hammersley(uint i, uint N)
    {
	    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
    }
    // ----------------------------------------------------------------------------
    vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
    {
	    float a = roughness*roughness;
	
	    float phi = 2.0 * PI * Xi.x;
	    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	    // from spherical coordinates to cartesian coordinates - halfway vector
	    vec3 H;
	    H.x = cos(phi) * sinTheta;
	    H.y = sin(phi) * sinTheta;
	    H.z = cosTheta;
	
	    // from tangent-space H vector to world-space sample vector
	    vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	    vec3 tangent   = normalize(cross(up, N));
	    vec3 bitangent = cross(N, tangent);
	
	    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	    return normalize(sampleVec);
    }
    // ----------------------------------------------------------------------------
    float GeometrySchlickGGX(float NdotV, float roughness)
    {
        // note that we use a different k for IBL
        float a = roughness;
        float k = (a * a) / 2.0;

        float nom   = NdotV;
        float denom = NdotV * (1.0 - k) + k;

        return nom / denom;
    }
    // ----------------------------------------------------------------------------
    float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
    {
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        float ggx2 = GeometrySchlickGGX(NdotV, roughness);
        float ggx1 = GeometrySchlickGGX(NdotL, roughness);

        return ggx1 * ggx2;
    }
    // ----------------------------------------------------------------------------
    vec2 IntegrateBRDF(float NdotV, float roughness)
    {
        //resolve the view vector  (in local coords)
        vec3 V;
        V.x = sqrt(1.0 - NdotV*NdotV);
        V.y = 0.0;
        V.z = NdotV;

        float A = 0.0;
        float B = 0.0; 

        vec3 N = vec3(0.0, 0.0, 1.0); //define an up, since we are working with n dot v a scalar angle
    
        const uint SAMPLE_COUNT = 1024u;
        for(uint i = 0u; i < SAMPLE_COUNT; ++i)
        {
            // generates a sample vector that's biased towards the
            // preferred alignment direction (importance sampling).
            vec2 Xi = Hammersley(i, SAMPLE_COUNT);
            vec3 H = ImportanceSampleGGX(Xi, N, roughness);
            vec3 L = normalize(2.0 * dot(V, H) * H - V); //sample vector

            float NdotL = max(L.z, 0.0);
            float NdotH = max(H.z, 0.0);
            float VdotH = max(dot(V, H), 0.0);

            if(NdotL > 0.0)
            {
                float G = GeometrySmith(N, V, L, roughness); //luckily, this function on depends on n dot v (n relative to v), not true V.
                float G_Vis = (G * VdotH) / (NdotH * NdotV);
                float Fc = pow(1.0 - VdotH, 5.0);

                A += (1.0 - Fc) * G_Vis;
                B += Fc * G_Vis;
            }
        }
        A /= float(SAMPLE_COUNT);
        B /= float(SAMPLE_COUNT);

        //return the intergal in the form F0*A + B
        return vec2(A, B);
        
    }
    // ----------------------------------------------------------------------------
    void main() 
    {
        vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);
        FragColor = integratedBRDF;
    }
    )";




    const char* fsLight = R"(
    #version 450 core
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
    #version 450 core
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
    #version 450 core
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
    #version 450 core
    out vec4 FragColor;
        
    in vec2 TexCoords;
        
    uniform sampler2D hdrScene; //0
    uniform float bloomThreshold;
    
    void main()
    {
        vec3 scene = texture(hdrScene, TexCoords).rgb;
        float luminance = dot(scene, vec3(0.2126, 0.7152, 0.0722));   
        
        vec3 result = luminance > bloomThreshold ? scene : vec3(0);    
        FragColor = vec4(result, 1.0f);
    }
 
    )";

    const char* fsBlur = R"(
    #version 450 core
    out vec4 FragColor;
        
    in vec2 TexCoords;
        
    uniform sampler2D scene;
    uniform bool horizontal;

    float weights[5] = float[] (0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f); //gaussian
        
    void main()
    {
        vec2 texOffset = 1.0f / textureSize(scene, 0); //size of one texel
        vec3 result = vec3(0.0f);
            
        //Contribution of the current fragment
        result += texture(scene, TexCoords).rgb * weights[0];
            
        if (horizontal)
        {
            for (int i = 1; i < 5; i++)
            {
                vec2 currentSampleOffset = vec2(texOffset.x * i, 0.0f);
                result += texture(scene, TexCoords + currentSampleOffset).rgb * weights[i];
                result += texture(scene, TexCoords - currentSampleOffset).rgb * weights[i];
            }
        }
        else
        {
            for (int i = 1; i < 5; i++)
            {
                vec2 currentSampleOffset = vec2(0.0f, texOffset.y * i);
                result += texture(scene, TexCoords + currentSampleOffset).rgb * weights[i];
                result += texture(scene, TexCoords - currentSampleOffset).rgb * weights[i];
            }
        }

        FragColor = vec4(result, 1.0f);
    }
 
    )";

    const char* vsDirShadow = R"(
    #version 450 core

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
    #version 450 core

    void main()
    {
	    gl_FragDepth = gl_FragCoord.z; //set depth buffer manually if we want (this is done auto though so who cares...)
    }
    )";

    const char* vsCascadedShadow = R"(
    #version 450 core

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
    #version 450 core

    void main()
    {
	    // gl_FragDepth = gl_FragCoord.z; //set depth buffer manually if we want (this is done auto though so who cares...)
    }
    )";

    const char* gsCascadedShadow = R"(
    #version 450 core

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
    #version 450 core

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
    #version 450 core

    in vec3 FragPos;

    uniform vec3 lightPos;
    uniform float pointShadowProjFarPlane;

    layout (location = 0) out vec2 FragColor;  
        
    void main()
    {
        float lightDistance = length(FragPos.xyz - lightPos);
    
        // map to [0,1] range by dividing by far_plane
        lightDistance = lightDistance / pointShadowProjFarPlane;
    
        //You can think of lightDistance as being an implicit function of the screen coordinates (screen_x, screen_y).
        //dFdx(value) estimates the rate of change of value between the current fragment and the fragment immediately to its right
        //dFdy(value) estimates the rate of change of value between the current fragment and the fragment immediately below it 
        float dx = dFdx(lightDistance);
        float dy = dFdx(lightDistance);
        
        //Remember: each pixel's depth is not constant if on a slope (especially low res shadowmaps)
        // this formula increases the variance based on the slope of the surface.
        // aka: adds the intra-pixel variance caused by the surface slope.
    
        float momentY = lightDistance * lightDistance + clamp(0.25f * (dx * dx + dy * dy), 0.0f, 0.0001f);
        FragColor = vec2(lightDistance, momentY); //r: d g: d^2
    }
    )";

    const char* vsSkybox = R"(
    #version 450 core
    
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
    #version 450 core
    layout (location = 0) out vec4 FragColor;

    in vec3 TexCoords;

    uniform samplerCube skyboxTexture;

    void main()
    {
	    FragColor = texture(skyboxTexture, TexCoords);
    }
    )";

    const char* vsTerrain = R"(
    #version 450 core
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
    #version 450 core
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
    #version 450 core
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
    #version 450 core

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
    #version 450 core

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
    #version 450 core
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
    #version 450 core
    
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
    #version 450 core
    
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
    #version 450 core
    
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

    const char* fsVSMPointBlur = R"(
    #version 450 core

    layout (location = 0) out vec2 FragColor;     

    in  vec2 TexCoords;

    uniform samplerCubeArray source;
    uniform bool   horizontal;
    uniform int    layer;          // cubemap-array layer

    // Maps (u,v) in the current cube-face’s 2D domain to a 3-D direction.
    vec3 dirFromFaceAndTexCoords(vec2 uv, int face)
    {
        uv = uv * 2.0 - 1.0;                    
        if (face == 0) return vec3( 1.0, -uv.y, -uv.x);
        if (face == 1) return vec3(-1.0, -uv.y,  uv.x);
        if (face == 2) return vec3( uv.x,  1.0,  uv.y);
        if (face == 3) return vec3( uv.x, -1.0, -uv.y);
        if (face == 4) return vec3( uv.x, -uv.y,  1.0);
                       return vec3(-uv.x, -uv.y, -1.0);   // face 5
    }

    // ---- constant kernel parameters ---------------------------------
    const int   R          = 5;               // kernel radius
    const int   TAP_COUNT  = R * 2 + 1;        // == 11

    // Normalised Gaussian weights for sigma = 3.0
    const float weights[TAP_COUNT] = float[](
    0.06634167, 0.07942539, 0.09136095, 0.10096946, 0.10721307,
    0.10937892,                       // centre tap (i = 0)
    0.10721307, 0.10096946, 0.09136095, 0.07942539, 0.06634167
    );

    // ---------------------------------------------
    // 1.  Centre vectors  C_f   (major axis ±1)
    // ---------------------------------------------
    const vec3 kFaceCentre[6] = vec3[](
        vec3( 1.0,  0.0,  0.0),   // +X   (face 0)
        vec3(-1.0,  0.0,  0.0),   // -X   (face 1)
        vec3( 0.0,  1.0,  0.0),   // +Y   (face 2)
        vec3( 0.0, -1.0,  0.0),   // -Y   (face 3)
        vec3( 0.0,  0.0,  1.0),   // +Z   (face 4)
        vec3( 0.0,  0.0, -1.0)    // -Z   (face 5)
    );
            
    // ---------------------------------------------
    // 2.  U-axis  (how local +u maps to world)
    // ---------------------------------------------
    const vec3 kFaceU[6] = vec3[](
        vec3( 0.0,  0.0, -1.0),   // face 0   
        vec3( 0.0,  0.0,  1.0),   // face 1  
        vec3( 1.0,  0.0,  0.0),   // face 2  
        vec3( 1.0,  0.0,  0.0),   // face 3  
        vec3( 1.0,  0.0,  0.0),   // face 4  
        vec3(-1.0,  0.0,  0.0)    // face 5
    );

    // ---------------------------------------------
    // 3.  V-axis  (how local +v maps to world)
    // ---------------------------------------------
    const vec3 kFaceV[6] = vec3[](
        vec3( 0.0, -1.0,  0.0),   // face 0  
        vec3( 0.0, -1.0,  0.0),   // face 1  
        vec3( 0.0,  0.0,  1.0),   // face 2  
        vec3( 0.0,  0.0, -1.0),   // face 3  
        vec3( 0.0, -1.0,  0.0),   // face 4  
        vec3( 0.0, -1.0,  0.0)    // face 5  
    );

    // -------------------------------------------------
    // Fast helper (no normalisation, no branching)
    // -------------------------------------------------
    vec3 dirFromFaceAndTexFast(vec2 uv, int face)
    {
        float u = uv.x * 2.0 - 1.0;
        float v = uv.y * 2.0 - 1.0;
        return kFaceCentre[face] + u * kFaceU[face] + v * kFaceV[face];
    }

    void main()
    {
        int  face       = layer % 6;
        int  lightIndex = layer / 6;

        vec2 texelSize = 1.0 / vec2(textureSize(source, 0));

        vec2  sum        = vec2(0.0);
        float weightSum  = 0.0;

        for (int i = -R; i <= R; ++i)
        {
            float w = weights[abs(i)];
            vec2 offset = horizontal ? vec2(texelSize.x * i, 0.0)
                                     : vec2(0.0, texelSize.y * i);

            vec3 dir = dirFromFaceAndTexFast(TexCoords + offset, face);
            vec2 sampleMoments = textureLod(source, vec4(dir, lightIndex), 0.0).rg;

            sum       += w * sampleMoments;
            weightSum += w;
        }

        FragColor = sum / weightSum;
    }

    )";

    const char* csSSBOTest = R"(
    #version 450 core
    
    layout(local_size_x = 1024) in;

    layout(std430, binding = 0) buffer Positions
    {
        vec4 positions[];
    };

    uniform float t;
    
    //gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID; <- this is globalinvocationID

    void main()
    {
        uint index = gl_GlobalInvocationID.x;
        
        /*
        float phase = (t/60) + float(index) * 0.01f;
        float radius = 0.5f + 0.5f * sin(phase);
        positions[index].xy = normalize(positions[index].xy) * radius;
        */

        float u = float(gl_GlobalInvocationID.x) / float(gl_NumWorkGroups.x * gl_WorkGroupSize.x);
        u = u * 2.0f - 1.0f; // [0,1] -> [-1,1]
        
        positions[index].x = u;
        float ampScale = (cos(t/4) + 1) / 2;
        positions[index].y = sin(u * 2.0f * 3.141f + (t/5)) * ampScale; 
    }
    )"; 

    const char* vsSSBOTest = R"(
    #version 450 core
    /*
    layout(std430, binding = 0) buffer Positions
    {
        vec4 positions[];
    };    
    */
    layout (location = 0) in vec4 aPos;

    void main()
    {
        // gl_VertexID ranges 0..N-1
        //gl_Position = positions[gl_VertexID];
        gl_Position = aPos;
        gl_PointSize = 4.0f;
    }
    )";

    const char* fsSSBOTest = R"(
    #version 450 core
    
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0f, 0.8f, 0.2f, 1.0f);
    }
    )";

    const char* csParticle = R"(
    #version 450 core
    
    layout(local_size_x = 1024) in;

    struct Particle
    {
        vec4 posLife;
        vec4 velActive;
    };

    layout(std430, binding = 1) buffer Particles
    {
        Particle particles[];
    };

    uniform float dt;
    uniform vec3 emitterPosition = vec3(0.0f, 0.0f, 0.0f); //model space
    uniform float maxLife;
    uniform float particleSpeed = 20.0f;
    uniform bool doneEmitting;

    uint WangHash(uint x)
    {
        x ^= x >> 16;
        x *= 0x7feb352d;
        x ^= x >> 15;
        x *= 0x846ca68b;
        x ^= x >> 16;
        return x;
    }

    float Random(uint seed) //[0,1]
    {
        return float(WangHash(seed)) / float(0xFFFFFFFFu);
    }

    void main()
    {
        uint id = gl_GlobalInvocationID.x;
        
        particles[id].posLife.w -= dt;

        //particle is dead
        if (particles[id].posLife.w <= 0.0f)
        {
            if (!doneEmitting)
            {
                uint seed = id + gl_WorkGroupID.x * 256;
                //respawn it
                particles[id].posLife.xyz = emitterPosition;
                particles[id].posLife.w = maxLife;
            
                //give it a random velocity
                vec3 rnd;
                rnd.x = Random(seed + 1) * 2.0f - 1.0f; //[-1,1]
                rnd.y = Random(seed + 2) + 0.75;        //[0,1]
                rnd.z = Random(seed + 3) * 2.0f - 1.0f; //[-1,1]

                particles[id].velActive.xyz = normalize(rnd) * particleSpeed * Random(seed + 4); 
                particles[id].velActive.w = 1.0f;
            }
            else
            {
                particles[id].velActive.w = -1.0f;
            }
        }
        //alive, time step
        else
        {
            particles[id].velActive.y -= (3.81f * dt);
            particles[id].posLife.xyz += (particles[id].velActive.xyz * dt);
        }
    }
    )";

    const char* vsParticle = R"(
    #version 450 core
    
    layout (location = 0) in vec3 aPos; //from VAO
    layout (location = 1) in float aLife; //from VAO
    layout (location = 2) in float aIsActive; //from VAO

    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 model;

    out float life;
    out float isActive;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0f);
        gl_PointSize = 0.5f;

        life = aLife;
        isActive = aIsActive;
    }
    )";

    const char* fsParticle = R"(
    #version 450 core
    
    out vec4 FragColor;
    
    in float life;
    in float isActive;

    void main()
    {
        if (isActive > 0.0f) FragColor = vec4(life, 3.0f - life, 0.0f, 1.);
        else discard;
    }
    )";

    const char* csTileCulling = R"(
    #version 450 core
    layout(local_size_x = 16, local_size_y = 16) in; //16x16 threads per work group
    
    struct GPUPointLight {    
        vec4 positionRange;   
        vec4 colorIntensity; 
        uint isActive;
        int shadowMapIndex;
        uint pad0, pad1; //48 
    };    
    
    layout(std430, binding = 1) buffer Lights
    {
        GPUPointLight lights[];  
    };

    layout(std430, binding = 2) buffer TileIndices
    {
        uint indices[];
    };
    
    layout(std430, binding = 3) buffer TileCount
    {
        uint counts[];
    };
    
    uniform float farPlane;

    uniform mat4 view;
    uniform mat4 projection;
    uniform vec2 screen; //w , h

    const uint MAX_LIGHTS_PER_TILE = 256u;
    const uint tileSize = 16u;
    uniform int numLights;
                 
    void main()
    {
        uvec2 tileCoords = gl_WorkGroupID.xy;
        uvec2 groupCount = uvec2( (screen + tileSize - 1u) / tileSize );

        if (tileCoords.x >= groupCount.x || tileCoords.y >= groupCount.y)
        {
            return;
        }

        uint tileID = tileCoords.y * groupCount.x + tileCoords.x;
        uint base = tileID * MAX_LIGHTS_PER_TILE;

        if (gl_LocalInvocationIndex == 0u) counts[tileID] = 0u;
        memoryBarrierBuffer();
        barrier();

        //corners
        vec2 pxMin = vec2(tileCoords) * tileSize;
        vec2 pxMax = pxMin + tileSize;
        
        //t1    -> lights[0],   lights[256], lights[512] ...
        //t2    -> lights[1],   lights[257], lights[513] ...
        // ...
        //t255  -> lights[255], lights[511], lights[767] ...
        for (uint i = gl_LocalInvocationIndex; i < uint(numLights); i += tileSize * tileSize)
        {
            GPUPointLight l = lights[i];
            if (l.isActive == 0u) continue;
            
            //IS LIGHT IN TILE CHECK-------------------------------------------
            vec4 viewPos = view * vec4(l.positionRange.xyz, 1.0f);
            float z = -viewPos.z;

            float radius = l.positionRange.w;
            float radiusScreenSpace = (radius / z) * projection[1][1] * screen.y * 0.5;
            
            vec4 clip = projection * viewPos;
            vec2 ndc = clip.xy / clip.w;
        
            //light position in pixel coords [(0,0), (SCREEN_WIDTH, SCREEN_HEIGHT)]
            vec2 center = (ndc * 0.5f + 0.5f) * screen;
            
            //nearest point to the light position on the current tile
            vec2 closest = clamp(center, pxMin, pxMax); 

            //squared distance between light pos and nearest point in the current tile
            float d2 = dot(center - closest, center - closest); 
            bool inside = d2 <= radiusScreenSpace * radiusScreenSpace;
            //-----------------------------------------------------------------   

            if (inside)
            {
                //NOTE: Without atomic add:
                //      uint old = counts[tileID];
                //      counts[tileID] = old + 1;
                //
                //BUT: But 2 threads can take the same old, losing a count update.
                //
                //index = counts[tileID] before increment
                uint index = atomicAdd(counts[tileID], 1);
                if (index < MAX_LIGHTS_PER_TILE)
                {
                    indices[base + index] = i;
                }        
            }
        }

        memoryBarrierBuffer(); 
        barrier();

        if (gl_LocalInvocationIndex == 0u)
        {
            counts[tileID] = min(counts[tileID], MAX_LIGHTS_PER_TILE);
        }
    }
    )";


}


/*
    vec3 L = fragPos - lightPos;
    float fragDepth = length(L); // [0, pointShadowProjFarPlane]

    float viewDistance = length(viewPos - fragPos);
    float diskRadius   = (1.0 + (viewDistance / pointShadowProjFarPlane)) / 25.0;

    float shadow = 0.0;
    const int samples = 20;

    for (int i = 0; i < samples; ++i)
    {
        vec3 sampDir = normalize(L + sampleOffsetDirections[i] * diskRadius);

        vec2 moments = texture(pointShadowMapArray, vec4(sampDir, float(index))).rg;
        float   E_d    = moments.x * pointShadowProjFarPlane;
        float   E_d2   = moments.y * pointShadowProjFarPlane * pointShadowProjFarPlane;

        float variance = max(E_d2 - (E_d * E_d), 0.00001);
        float d        = fragDepth - E_d;
        float pMax     = variance / (variance + d * d);

        if (fragDepth <= E_d) shadow += 1.0;
        else shadow += pMax;
    }
    shadow = shadow / float(samples);
    return clamp(shadow, 0.0, 1.0);
*/

//PCF---
/*
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
*/

//color test compute shader
/*
    const char* csColorTest = R"(
    #version 450 core

    layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

    layout(rgba32f, binding = 0) uniform image2D imgOutput;

    uniform float t;

    void main()
    {
        vec4 value = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);

        value.r = mod(float(texelCoord.x) + t * 10, 512) / (gl_NumWorkGroups.x * gl_WorkGroupSize.x);
        value.g = float(texelCoord.y)/(gl_NumWorkGroups.y * gl_WorkGroupSize.y);

        imageStore(imgOutput, texelCoord, value);
    }

    )";
*/