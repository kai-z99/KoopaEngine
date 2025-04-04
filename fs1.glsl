#version 410 core
layout (quads, fractional_odd_spacing, cw) in;

uniform sampler2D heightMap;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const float heightScale = 64.0f;
const float heightOffset = -16.0f;

in vec2 TexCoords_TCS_OUT[]; //array of texture coords of the current patch where this current vertex is located.
                    //since its quads, one in each corner. (Control points)
        
out vec3 Normal;
out vec2 TexCoords;
out vec3 FragPos;
out float Height;
out mat3 TBN;

const vec2 worldTexelSize = vec2(1.0f, 1.0f);
//For simplicity, texturesize is equivalnt to world size.

        

void main()
{
    //get tess coords [0,1]
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    //texcoord control points
    vec2 t00 = TexCoords_TCS_OUT[0];
    vec2 t01 = TexCoords_TCS_OUT[1];
    vec2 t10 = TexCoords_TCS_OUT[2];
    vec2 t11 = TexCoords_TCS_OUT[3];

    //bilinear interpolate the tessCoord texCoord from control points
    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 currentTexCoord = (t1 - t0) * v + t0; //the texcoord of the current vertex based of its control points.
            
    //the height of this vertex.
    float currentHeight = texture(heightMap, currentTexCoord).r * heightScale + heightOffset;
            
    //position control points
    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;
            
    //compute normals
    vec4 uVec = p01 - p00;
    vec4 vVec = p10 - p00;
    vec4 patchNormal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) ); //should be 0,1,0
            
    //bilinear interpolate the vertexes position from control points
    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 position = (p1 - p0) * v + p0;
    position += patchNormal * currentHeight;

    //NORMAL CALCLUATION - FINITE DIFFERENCES -------------------
    vec2 texelSize = 1.0f / textureSize(heightMap, 0);
    //Get texCoords for neighbors
    vec2 texCoord_L = currentTexCoord - vec2(texelSize.x, 0);
    vec2 texCoord_R = currentTexCoord + vec2(texelSize.x, 0);
    vec2 texCoord_B = currentTexCoord - vec2(0, texelSize.y);
    vec2 texCoord_T = currentTexCoord + vec2(0, texelSize.y);

    //Get heights of neighboring texels
    float height_L = texture(heightMap, texCoord_L).r * heightScale + heightOffset;
    float height_R = texture(heightMap, texCoord_R).r * heightScale + heightOffset;
    float height_B = texture(heightMap, texCoord_B).r * heightScale + heightOffset;
    float height_T = texture(heightMap, texCoord_T).r * heightScale + heightOffset;

    //Calculate tangent vectors
    //dX = 2.0*1.0 = 2.0 (assuming U maps to X)
    vec3 tangentU = vec3(2.0 * worldTexelSize.x, height_R - height_L, 0.0);
    //dZ = 2.0*1.0 = 2.0 (assuming V maps to -Z)
    vec3 tangentV = vec3(0.0, height_T - height_B, 2.0 * worldTexelSize.y); 

    //world space Normal/Tangent
    vec3 normalLocal = cross(tangentU, tangentV);

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 worldNormal = normalize(normalMatrix * normalLocal);
    
    //calculate TBN matrix
    vec3 tangentLocal = normalize(tangentU - dot(tangentU, normalLocal) * normalLocal);

    vec3 worldTangent = normalize(normalMatrix * tangentLocal);
    vec3 worldBiTangent = normalize(cross(worldNormal, worldTangent));

    mat3 tbn = mat3(worldTangent,worldBiTangent,worldNormal);

    //FINAL OUT VALUES
    gl_Position = projection * view * model * position;
    FragPos = vec3(model * position);
    Normal = worldNormal;
    TexCoords = currentTexCoord;
    Height = currentHeight;
    TBN = tbn;
}