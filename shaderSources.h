    #pragma once

    namespace ShaderSources
    {
        const char* vs1 = R"(
        #version 410 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;
        layout (location = 3) in vec3 aTangent;
        layout (location = 4) in vec3 aBitangent;
        
        //UNIFORMS ------------------------------------------------------------------------------------
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform mat4 dirLightSpaceMatrix;

        //OUT VARIABLES--------------------------------------------------------------------------------
        out vec2 TexCoords;
        out vec3 FragPos; //Frag pos in world position
        out vec3 Normal;
        out mat3 TBN; //TangentSpace -> WorldSpace
        out vec4 FragPosDirLightSpace;

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

            FragPosDirLightSpace = dirLightSpaceMatrix * model * vec4(aPos, 1.0);
        }
        )";

        const char* fs1 = R"(
        #version 410 core
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
    
        float DirShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir);
        float CascadeShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir);
        float PointShadowCalculation(vec3 fragPos, vec3 lightPos, int index);
        
        //IN VARIABLES--------------------------------------------------------------------------------
        in vec2 TexCoords;
        in vec3 Normal;
        in vec3 FragPos;
        in mat3 TBN;
        in vec4 FragPosDirLightSpace;
        
        //SAMPLERS------------------------------------------------------------------------------------
        //material
        uniform sampler2D currentDiffuse;               //0
        uniform sampler2D currentNormalMap;             //1
        //shadowmaps
        uniform sampler2D dirShadowMap;                 //2
        uniform samplerCubeArray pointShadowMapArray;   //3
        uniform sampler2DArray cascadeShadowMaps;       //4
        //model
        uniform sampler2D texture_diffuse1;              //5-8
        uniform sampler2D texture_specular1;             //5-8
        uniform sampler2D texture_normal1;               //5-8
        uniform sampler2D texture_height1;               //5-8
        
        //UNIFORMS------------------------------------------------------------------------------------

        //cascade
        uniform float cascadeDistances[4];              //compile time
        uniform int cascadeCount;                       //compile time
        uniform mat4 cascadeLightSpaceMatrices[5];      //runtime
 
        //mesh

        //params/data
        uniform bool usingModel;

        uniform float farPlane; //for point shadow calculation
        uniform vec3 baseColor; //Use this if not using a diffuse texture
        uniform vec3 viewPos;
        uniform bool usingNormalMap;
        uniform bool usingDiffuseMap;
        uniform mat4 view;
        

        void main()
        {
            vec3 color = vec3(0.0f);
            vec3 viewDir = normalize(viewPos - FragPos);
            float gamma = 2.2;
            //make sure to convert diffuse to linear space
            
            //Find which textures to use
            vec3 diffuse;
            vec3 normal;

            if (usingModel)
            {
                diffuse = pow(texture(texture_diffuse1, TexCoords).rgb, vec3(gamma));   

                normal = texture(texture_normal1, TexCoords).rgb;
                normal = normal * 2.0f - 1.0f; //[0,1] -> [-1, 1]
                normal = normalize(TBN * normal); //Tangent -> World (tbn is constucted with model matrix)
                normal = normalize(Normal);
            }
            else
            {
                if (usingDiffuseMap)
                {
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
            
            //BIAS (0.00035 is optimal, no culling)
            float bias = max(0.0035 * (1.0 - dot(normal, lightDir)), 0.00035); //more bias with more angle.
            if (layer == cascadeCount) bias *= 1 / (farPlane * 0.5f);
            else bias *= 1 / (cascadeDistances[layer] * 0.5f);

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

            float fragDepth = length(lightToFrag); // [0, farPlane]
            float closestDepth = texture(pointShadowMapArray, vec4(lightToFrag, index)).r;// [0,1]
            closestDepth *= farPlane; //[0,1] -> [0, farPlane]
        
            float shadow = 0.0f;
            float bias = max(0.05 * (1.0 - dot(normalize(normal), normalize(lightToFrag))), 0.005);  
            
            //PCF---
            int samples = 20;
            float viewDistance = length(viewPos - fragPos);
            float diskRadius = (1.0 + (viewDistance / farPlane)) / 25.0; 
    
            for(int i = 0; i < samples; ++i)
            {
                float closestDepth = texture(pointShadowMapArray, vec4(lightToFrag + sampleOffsetDirections[i] * diskRadius, index)).r;
                closestDepth *= farPlane;   // move to [0, farPlane]

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
            vec3 specular = light.intensity * light.color  * spec; // * vec3(texture(material.specular, TexCoord)); not using map rn

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
            vec3 specular = light.intensity * light.color  * spec * (usingModel ? vec3(texture(texture_specular1, TexCoords)) : 1.0f);
        
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
        )";

        const char* fsLight = R"(
        #version 410 core
        layout (location = 0) out vec4 FragColor; //COLOR_ATTACHEMNT0
        layout (location = 1) out vec4 BrightColor; //COLOR_ATTACHEMNT1
        
        uniform vec3 lightColor;
        uniform float intensity;

        void main()
        {
            FragColor = vec4(intensity * lightColor, 1.0); // a cube, that doesnt acutally emit light remember its purely a visual
            
            float brightness = dot(FragColor.rgb, vec3(0.2126f, 0.7152f, 0.0722f)); //Luminance formula, based on human vision
            if (brightness > 1.0) BrightColor = vec4(lightColor, 1.0f);
            else BrightColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
        )";

        const char* vsScreenQuad = R"(
        #version 410 core
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
        #version 410 core
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

        const char* fsBlur = R"(
        #version 410 core
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
        #version 410 core

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
        #version 410 core

        void main()
        {
	        gl_FragDepth = gl_FragCoord.z; //set depth buffer manually if we want (this is done auto though so who cares...)
        }
        )";

        const char* vsCascadedShadow = R"(
        #version 410 core

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
        #version 410 core

        void main()
        {
	       // gl_FragDepth = gl_FragCoord.z; //set depth buffer manually if we want (this is done auto though so who cares...)
        }
        )";

        const char* gsCascadedShadow = R"(
        #version 410 core

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
        #version 410 core

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
        #version 410 core

        in vec3 FragPos;

        uniform vec3 lightPos;
        uniform float farPlane;

        void main()
        {
            float lightDistance = length(FragPos.xyz - lightPos);
    
            // map to [0,1] range by dividing by far_plane
            lightDistance = lightDistance / farPlane;
    
            // write this as modified depth
            gl_FragDepth = lightDistance;
        }
        )";

        const char* vsSkybox = R"(
        #version 410 core
    
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
        #version 410 core
        layout (location = 0) out vec4 FragColor;

        in vec3 TexCoords;

        uniform samplerCube skyboxTexture;

        void main()
        {
	        FragColor = texture(skyboxTexture, TexCoords);
        }
        )";
        

        //const char fsSkybox[] = {
        //#include "fs1.glsl"
        //};
    }
