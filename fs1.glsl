    #version 420 core
    out vec4 FragColor;
        
    in vec2 TexCoords;
        
    uniform sampler2D hdrScene; //0
    uniform float bloomThreshold;

    void main()
    {
        vec3 scene = texture(hdrScene, TexCoords).rgb;
        float luminance = dot(scene, vec3(0.2126, 0.7152, 0.0722));   
        
        vec3 out = luminance > bloomThreshold ? scene : vec3(0.0f);    
        FragColor = vec4(out, 1.0f);
    }