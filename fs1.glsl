    #version 450 core
    
    layout(local_size_x = 256) in;

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

        positions[index].x = (gl_GlobalInvocationID.x/(gl_NumWorkGroups.x * gl_WorkGroupSize.x));
        positions[index].y = sin((gl_GlobalInvocationID.x/(gl_NumWorkGroups.x * gl_WorkGroupSize.x)) * t); 
    }