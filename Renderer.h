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
    
    void ClearScreen(Vec4 col);

    void SetCurrentDiffuse(const char* path);

	void DrawTriangle(Vec3 pos = { 0,0,0 }, Vec4 rotation = { 0,1,0,0 });
	void DrawCube(Vec3 pos = { 0,0,0 }, Vec3 size = { 1,1,1 }, Vec4 rotation = { 0,1,0,0 });
    void DrawPlane(Vec3 pos = { 0,0,0 }, Vec3 size = { 1,1,1 }, Vec4 rotation = { 0,1,0,0 });

    void DrawLightsDebug();

    void AddPointLightToFrame(Vec3 pos = { 0,0,0 },
                            Vec3 col = { 0,0,0 },
                            float intensity = 1.0f);

    void SetAllPointLightsToFalse();

    void SetCameraMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position);

    
private:
    const unsigned int MAX_POINT_LIGHTS = 4;

    Shader* shader; 
    Shader* debugLightShader;

    unsigned int currentDiffuseTexture;
    unsigned int LoadTexture(const char* path);
    std::unordered_map<const char*, unsigned int> textureToID;

    unsigned int currentFramePointLightCount;
    struct PointLight
    {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        bool isActive;
    };
    PointLight pointLights[4];
    void SetPointLightProperties(unsigned int index, Vec3 pos, Vec3 col, float intensity);
    void InitializePointLights();

    void SetupVertexBuffers();
    void SetupTriangleBuffers();
    void SetupCubeBuffers();
    void SetupPlaneBuffers();
    unsigned int triangleVAO, triangleVBO, planeVBO;
    unsigned int cubeVAO, cubeVBO, planeVAO;

    void SetupFramebuffers();
    void SetupFinalImageFramebuffer();
    //void SetupShadowMap framebuffer ...
    // .. .. 

};