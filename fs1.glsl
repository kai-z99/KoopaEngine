
#version 420 core
layout (location = 0) out vec4 FragColor;   //COLOR_ATTACHMENT_0
layout (location = 1) out vec4 BrightColor; //COLOR_ATTACHMENT_1

struct PointLight {    
    vec3 position;   
    vec3 color; 
    float intensity;
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
uniform float cascadeDistances[3];              //set once in constructor
uniform int cascadeCount;                       //set once in constructor
uniform mat4 cascadeLightSpaceMatrices[4];      //updated every frame in RenderCascadedShadowMap()
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
            
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) BrightColor = vec4(color, 1.0f);
    else BrightColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
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
    float bias = max(0.00025 * (1.0 - dot(normal, lightDir)), 0.000025); //more bias with more angle.
      
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
    float bias = max(0.05 * (1.0 - dot(normalize(normal), normalize(lightToFrag))), 0.005);  
    float slope = (1.0 - dot(normalize(normal), normalize(lightToFrag));
    bias = max((0.0005 + 0.002 * slope) , (fragDepth / 45.0f)); 
            
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
    float attenuation = 1.0 / (distance * distance); //quadratic attenuation

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