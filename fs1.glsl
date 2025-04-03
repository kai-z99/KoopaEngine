        #version 410 core
        layout (vertices = 4) out;

        in vec2 TexCoords_VS_OUT[]; //since were working in patches
        out vec2 TexCoords_TCS_OUT[]; //likewise
            
        const int MIN_TESS_LEVEL = 4;
        const int MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE = 15.0f;
        const float MAX_DISTANCE = 200.0f;

        void main()
        {
            //NOTE: gl_in[] is a built-in, read-only array that contains all the vertices of the current patch. 
            //      (the data includes position and other per-vertex attributes)

            //NOTE: gl_in[0] : TL
                    gl_in[1] : TR
                    gl_in[2] : BL
                    gl_in[3] : BR
                        Since we defined them in this order in vertex buffer creation. (See cpp code)

            //pass through
            gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
            TexCoords_TCS_OUT[gl_InvocationID] = TexCoords_VS_OUT[gl_InvocationID];
            //----

            //0 controls tessellation level
            if (gl_InvocationID == 0)
            {
                //TL
                vec4 viewSpacePos00 = view * model * gl_in[0].gl_Position; //see note but this is the position of a vertex of the current patch
                //TR
                vec4 viewSpacePos01 = view * model * gl_in[1].gl_Position;
                //BL
                vec4 viewSpacePos10 = view * model * gl_in[2].gl_Position;
                //BR
                vec4 viewSpacePos11 = view * model * gl_in[3].gl_Position;
                
                //note: distance from the camera [0,1]
                //TL
                float distance00 = clamp((abs(viewSpacePos00.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);
                //TR
                float distance01 = clamp((abs(viewSpacePos01.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);
                //BL
                float distance10 = clamp((abs(viewSpacePos10.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);
                //BR
                float distance11 = clamp((abs(viewSpacePos11.z) - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);

                //interpolate tesselation based on closer vertex

                //NOTE: OL-0  is left side of cube. So take the min of the distances of the TL and BL vertices.
                float tessLevel0 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance10));
                //Ol-1: bottom. BL and BR
                float tessLevel1 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance10, distance11));
                //Ol-2: right. BR and TR
                float tessLevel2 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11));
                //Ol-3: top. TL and TR
                float tessLevel3 = mix(MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01));
                

                gl_TessLevelOuter[0] = tessLevel0;
                gl_TessLevelOuter[1] = tessLevel1;
                gl_TessLevelOuter[2] = tessLevel2;
                gl_TessLevelOuter[3] = tessLevel3;
                
                gl_TessLevelInner[0] = max(tessLevel1, tessLevel3); //inner top/bot
                gl_TessLevelInner[1] = max(tessLevel0, tessLevel2); //inner l/r
            }
        }