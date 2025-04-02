        #version 410 core
        layout (quads, fractional_odd_spacing, ccw) in;

        uniform sampler2D heightMap;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        in vec2 TexCoord_TCS_OUT[]; //array of texture coords of the current patch where this current vertex is located.
                            //since its quads, one in each corner. (Control points)

        out vec3 Normal;
        out vec2 TexCoords;
        out vec3 FragPos;
        out float Height;
        
        void main()
        {
            float u = gl_TessCoord.x;
            float v = gl_TessCoord.y;

            //texcoord control points
            vec2 t00 = TexCoord_TCS_OUT[0];
            vec2 t01 = TexCoord_TCS_OUT[1];
            vec2 t10 = TexCoord_TCS_OUT[2];
            vec2 t11 = TexCoord_TCS_OUT[3];

            //bilinear interpolate the tessCoord texCoord from control points
            vec2 t0 = (t01 - t00) * u + t00;
            vec2 t1 = (t11 - t10) * u + t10;
            vec2 texCoord = (t1 - t0) * v + t0; //the texcoord of the current vertex based of its control points.
            
            Height = texture(heightMap, texCoord).y * 64.0f - 16.0f;
            
            //position control points
            vec4 p00 = gl_in[0].gl_Position;
            vec4 p01 = gl_in[1].gl_Position;
            vec4 p10 = gl_in[2].gl_Position;
            vec4 p11 = gl_in[3].gl_Position;
            
            //compute normals
            vec4 uVec = p01 - p00;
            vec4 vVec = p10 - p00;
            vec4 normal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) );
            
            //bilinear interpolate the vertexes position from control points
            vec4 p0 = (p01 - p00) * u + p00;
            vec4 p1 = (p11 - p10) * u + p10;
            vec4 position = (p1 - p0) * v + p0;
            
            position += normal * Height;
            
            gl_Position = projection * view * model * position;
            FragPos = vec3(model * vec4(position, 1.0));
            Normal = normalize(vec3(normal));
        }