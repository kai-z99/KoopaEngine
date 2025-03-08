#pragma once

#include "KoopaMath.h"
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

class Shader;

class Renderer
{
public:
	Renderer();
	~Renderer();
    
    void BeginRenderFrame();
    void EndRenderFrame();

    void ClearScreen(Vec4 col);

    void SetCurrentDiffuse(const char* path);
    void SetCurrentNormal(const char* path);

	void DrawTriangle(Vec3 pos = { 0,0,0 }, Vec4 rotation = { 0,1,0,0 });
	void DrawCube(Vec3 pos = { 0,0,0 }, Vec3 size = { 1,1,1 }, Vec4 rotation = { 0,1,0,0 });
    void DrawPlane(Vec3 pos = { 0,0,0 }, Vec2 size = { 1,1 }, Vec4 rotation = { 0,1,0,0 });

    void DrawLightsDebug();

    void AddPointLightToFrame(Vec3 pos,Vec3 col, float intensity);

    void AddDirLightToFrame(Vec3 dir, Vec3 col, float intensity);

    void SetAllPointLightsToFalse();

    void SetCameraMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position);

    
private:
    const unsigned int MAX_POINT_LIGHTS = 4;

    Shader* shader; 
    Shader* debugLightShader;
    Shader* screenShader;

    //TEXTURES
    unsigned int currentDiffuseTexture;
    unsigned int currentNormalMapTexture;
    unsigned int LoadTexture(const char* path);
    std::unordered_map<const char*, unsigned int> textureToID;
    void AddToTextureMap(const char* path); //stores texture to map if its not already there


    //LIGHTING
    unsigned int currentFramePointLightCount;
    struct PointLight
    {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        bool isActive;
    };
    PointLight pointLights[4];
    void SetNextPointLightProperties(unsigned int index, Vec3 pos, Vec3 col, float intensity);
    void InitializePointLights();

    struct DirLight
    {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        bool isActive;
    };
    DirLight dirLight;
    void SetDirLightProperties(Vec3 dir, Vec3 col, float intensity);
    void InitializeDirLight();

    //VERTEX BUFFER/ARRAY
    void SetupVertexBuffers();
    void SetupTriangleBuffers();
    void SetupCubeBuffers();
    void SetupPlaneBuffers();
    void SetupScreenQuadBuffers();
    unsigned int triangleVAO, triangleVBO, planeVBO;
    unsigned int cubeVAO, cubeVBO, planeVAO;
    unsigned int screenQuadVBO, screenQuadVAO;

    //FRAMEBUFFERS
    void SetupFramebuffers();
    void SetupFinalImageFramebuffer();
    unsigned int finalImageFBO; //framebuffers
    unsigned int finalImageTextureColorBuffer;   //texture
    unsigned int finalImageRBO;                 //renderbuffers

    //void SetupShadowMap framebuffer ...
    // .. .. 

};