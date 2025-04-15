    #version 420 core
    
    in vec2 TexCoords;

    uniform sampler2D gNormal;              //0
    uniform sampler2D gPosition;            //1
    uniform sampler2D ssaoNoiseTexture;     //2

    uniform vec3 samples[64];            //hemisphere vectors in tangent space
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
        
        for (int i = 0; i < 64; i++)
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
        
        occlusion = 1.0f - (occlusion / 64);
	    FragColor = occlusion;
    }