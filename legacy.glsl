
//VERSION WITH DIRECTIONAL SHADOW
/*
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
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, int index);
 
struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
    bool isActive;    
    bool castShadows;
};
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor);
    
//float DirShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
float CascadeShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir);
float PointShadowCalculation(vec3 fragPos, vec3 lightPos, int index);
        
//IN VARIABLES--------------------------------------------------------------------------------
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in mat3 TBN;
        
//SAMPLERS------------------------------------------------------------------------------------
uniform sampler2D currentDiffuse;                //0
uniform sampler2D currentNormalMap;              //1
uniform sampler2D dirShadowMap;                  //2
uniform samplerCubeArray pointShadowMapArray;    //3
uniform sampler2DArray cascadeShadowMaps;        //4
uniform sampler2D texture_diffuse1;              //5-8
uniform sampler2D texture_specular1;             //5-8
uniform sampler2D texture_normal1;               //5-8
uniform sampler2D texture_height1;               //5-8
//TERRAIN TEXTURE                                //9
    
//TODO: If using UBOs.



//SHARED UNIFORMS---------------------------------------------------------------------
uniform PointLight pointLights[4];
uniform int numPointLights;
uniform DirLight dirLight;

uniform float cascadeDistances[3];              //set once in constructor
uniform int cascadeCount;                       //set once in constructor
uniform mat4 cascadeLightSpaceMatrices[4];      //updated every frame in RenderCascadedShadowMap()
uniform float farPlane;                         //updated every frame in SendCameraUniforms() (unnessesarily)
uniform float pointShadowProjFarPlane;          //for point shadow calculation
uniform vec3 viewPos;                           //updated every frame in SendCameraUniforms() 
uniform mat4 view;                              //updated every frame in SendCameraUniforms() 

//UNIQUE UNIFORMS (Set by SendUniqueUniforms())-------------------------------------
uniform bool usingModel;
uniform bool modelMeshHasNormalMap;
uniform vec3 baseColor;    //Use this if not using a diffuse texture
uniform bool usingNormalMap;
uniform bool usingDiffuseMap;
uniform float specularIntensity;

void main()
{
    vec3 color = vec3(0.0f);
    vec3 viewDir = normalize(viewPos - FragPos);
    float gamma = 2.2;

    //Find which textures to use
    vec3 diffuse;
    vec3 normal;

    if (usingModel)
    {
        diffuse = pow(texture(texture_diffuse1, TexCoords).rgb, vec3(gamma));   

        if (modelMeshHasNormalMap) //if the current mesh being drawn has a normal map, use that.
        {
            normal = texture(texture_normal1, TexCoords).rgb;
            normal = normal * 2.0f - 1.0f; //[0,1] -> [-1, 1]
            normal = normalize(TBN * normal); //Tangent -> World (tbn is constucted with model matrix)
        }
        else //otherwise use the normals in the vertex data.
        {
            normal = normalize(Normal); 
        }
    }
    else
    {
        if (usingDiffuseMap)
        {
            //convert to linear space
            diffuse = pow(texture(currentDiffuse, TexCoords).rgb, vec3(gamma));
        }
        else
        {
            diffuse = baseColor;
        }

        if (usingNormalMap)
        {
            normal = texture(currentNormalMap, TexCoords).rgb;
            normal = normal * 2.0f - 1.0f; //[0,1] -> [-1, 1]
            normal = normalize(TBN * normal); //Tangent -> World (tbn is constucted with model matrix)
        }
        else
        {
            normal = normalize(Normal);
        }                
    }
            
    //LIGHTS
    for (int i = 0; i < numPointLights; i++)
    {
        color += CalcPointLight(pointLights[i], normal, FragPos, viewDir, diffuse, i);
    }

    color += CalcDirLight(dirLight, normal, FragPos, viewDir, diffuse);
    
    //Ambient lighting
    vec3 sceneAmbient = vec3(0.015f, 0.015f, 0.015f) * diffuse; 
    color += sceneAmbient;

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
    float bias = max(0.00015 * (1.0 - dot(normal, lightDir)), 0.000015); //more bias with more angle.
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

        

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor, int index)
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
    vec3 specular = light.intensity * light.color  * spec * (usingModel ? vec3(texture(texture_specular1, TexCoords).r) : vec3(specularIntensity));

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

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseColor)
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
    vec3 specular = light.intensity * light.color  * spec * (usingModel ? vec3(texture(texture_specular1, TexCoords).r) : vec3(specularIntensity));
        
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

/*
float DirShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    //note: we only care about z comp of fragPosLightSpace.
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; //perspective divide to convert to [-1,1]. 
    //Now: {[-1,1], [-1,1], [-1,1]} (if in frustum, AKA will show in the shadow snapshot, not clipped out)

    projCoords = projCoords * 0.5 + 0.5;  
    //Now: {[0,1] , [0,1], [0,1]} (if in frustum, AKA will show in the shadow snapshot, not clipped out)
            
    float fragDepth = projCoords.z;

    float closestDepth = texture(dirShadowMap, projCoords.xy).r; //note: this can sample outside [0,1] if its outsidde the frag was outside frustum. (clipped out)
    //if it does, it return 1.0f due to the wrapping method we set.
            

    if (fragDepth > 1.0f) return 0.0f; //Outside the far plane

    float bias = max(0.0025 * (1.0 - dot(normal, lightDir)), 0.00125); //more bias with more angle.
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
*/
