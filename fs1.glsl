    #version 450 core
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
    uniform int numPointLightsPlus;
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
    uniform uint tileSize = 16;
    uniform uvec2 screen = uvec2(1920, 1080);
    const int MAX_LIGHTS_PER_TILE = 128;

    //UNIQUE UNIFORMS --------------------------------------------------------------------
    //material properties (Set by SendMaterialUniforms())
    uniform Material material;
    uniform bool hasAlpha;
    uniform bool usingDiffuseMap;
    uniform bool usingNormalMap;
    uniform bool usingSpecularMap;

    uniform bool useSSS = false;
    uniform vec3 sssColor = vec3(0.02f, 0.42f, 0.02f);
    uniform float sigmaT = 1500.0f; //1500

    vec3 ChooseDiffuse();
    vec3 ChooseNormal();
    vec3 ChooseSpecular();
    float CalculateFogFactor();
    float CalculateLinearFog();

    const float gamma = 2.2;

    
    struct GPUPointLight    
    {    
        vec4 positionRange;   
        vec4 colorIntensity; 
        uint isActive;
        uint castShadows;
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

    void main()
    {
        vec3 color = vec3(0.0f);
        vec3 viewDir = normalize(viewPos - FragPos);    
        
        //Find which textures to use
        vec3 diffuse = ChooseDiffuse();
        vec3 normal = ChooseNormal();
        vec3 specular = ChooseSpecular();

        uvec2 pix = uvec2(gl_FragCoord.xy);
        uvec2 tile = pix / tileSize;
        uint tileID = tile.y * uint(screen.x / 16) + tile.x;

        //LIGHTS
        for (int i = 0; i < numPointLightsPlus; i++)
        {
            uint lightIndex = indices[tileID * MAX_LIGHTS_PER_TILE + i];

            GPUPointLight g = lights[lightIndex];
            PointLight p;
            p.position   = g.positionRange.xyz;
            p.range      = g.positionRange.w;
            p.color      = g.colorIntensity.rgb;
            p.intensity  = g.colorIntensity.a;
            p.isActive   = (g.isActive   != 0u);
            p.castShadows= (g.castShadows!= 0u);

            color += CalcPointLight(p, FragPos, viewDir, diffuse, normal, specular, i);
        }

        color += CalcDirLight(dirLight, FragPos, viewDir, diffuse, normal, specular);

        /*
        for (int i = 0; i < numPointLights; i++)
        {
            color += CalcPointLight(pointLights[i], FragPos, viewDir, diffuse, normal, specular, i);
        }
        
        */    
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

        if (hasAlpha) FragColor = FragColor = vec4(color, texture(material.diffuse, TexCoords).a);
        else FragColor = vec4(color, 1.0f);
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

        float shadow = LightBleedReduction(pMax, 0.25f);
        return clamp(shadow, 0.0f, 1.0f); 
    }

    vec3 CalcPointLight(GPUPointLight light, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, vec3 normal, vec3 baseSpecular, int index)
    {
        if (light.isActive == 0.0)
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
        float maxDistance = (farPlane / 7.5f) * (light.positionRange.w / 100.0f); //0: no light, 100: max reach
        if (maxDistance <= 0.0f) return vec3(0.0f);
        
        float t = clamp(1.0f - (distance / maxDistance), 0.0f, 1.0f);
        float attenuation = t * t; //quadratic attenuation
        // combine results
        vec3 diffuse  = light.colorIntensity.w * light.colorIntensity.rgb  * diff * diffuseColor;
        vec3 specular = light.colorIntensity.w * light.colorIntensity.rgb  * spec * baseSpecular;

        diffuse  *= attenuation;
        specular *= attenuation;
        
        if (light.castShadows == 0.0u)
        {
            return (diffuse + specular);
        }
        else
        {
            float shadow = PointShadowCalculation(fragPos, light.position.xyz, normal, index);
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