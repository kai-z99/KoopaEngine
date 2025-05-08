    #version 450 core
    layout(local_size_x = 16, local_size_y = 16) in;
    
    struct PointLight {    
        vec3 position;   
        vec3 color; 
        float intensity;
        float range;
        bool isActive;
        bool castShadows;
    };    

    layout(std430, binding = 1) buffer Lights
    {
        PointLight lights[];  
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

    uint MAX_LIGHTS_PER_TILE = 128;

    void main()
    {
        uvec2 tileCoords = gl_GlobalInvocationID.xy;
        if (tileCoords.x >= uint(screen.x)/16 || tileCoords.y >= uint(screen.y)/16 )
        {
            return;
        }

        uint tileID = tileCoords.y * uint(screen.x / 16) + tileCoords.x;
        uint base = tileID * MAX_LIGHTS_PER_TILE;

        counts[tileID] = 0;
        
        //corners
        vec2 px0 = vec2(tileCoords) * 16.0f;
        vec2 px1 = px0 + 16.0f;

        mat4 inverseVP = inverse(projection * view);
        
        for (uint i = 0; i < lights.length(); i++)
        {
            PointLight l = lights[i];
            
            vec4 viewPos = view * vec4(l.position, 1.0f);
            float z = -viewPos.z;

            float radius = (farPlane / 7.5f) * (l.range / 100.0f);
            float radiusScreenSpace = (radius / z) * projection[1][1] * screen.y * 0.5;
            
            vec4 clip = projection * viewPos;
            vec2 ndc = clip.xy / clip.w;
            vec2 center = (ndc * 0.5f + 0.5f) * screen;
            
            vec2 closest = clamp(center, px0, px1);
            float d2 = distance(center, closest);
            bool inside = d2 < radiusScreenSpace;

            if (inside)
            {
                uint index = atomicAdd(counts[tileID], 1);
                if (index < MAX_LIGHTS_PER_TILE)
                {
                    indices[base + index] = i;
                }        
            }
        }
    }