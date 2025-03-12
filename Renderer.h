#pragma once

#include "KoopaMath.h"
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

class Shader;
class DrawCall;

class Renderer
{
public:
    //Construction
	Renderer();
	~Renderer();
    
    //Begin/End frame
    void BeginRenderFrame();
    void EndRenderFrame();

    //Geometry drawing functions
    void SetCameraMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position);
    void ClearScreen(Vec4 col);
    void DrawTriangle(Vec3 pos = { 0,0,0 }, Vec4 rotation = { 0,1,0,0 });
    void DrawCube(Vec3 pos = { 0,0,0 }, Vec3 size = { 1,1,1 }, Vec4 rotation = { 0,1,0,0 });
    void DrawPlane(Vec3 pos = { 0,0,0 }, Vec2 size = { 1,1 }, Vec4 rotation = { 0,1,0,0 });

    //Modification functions
    void SetCurrentDiffuse(const char* path);
    void SetCurrentColorDiffuse(Vec3 col);
    void SetCurrentNormal(const char* path);

	//Light functions
    void DrawLightsDebug();
    void AddPointLightToFrame(Vec3 pos,Vec3 col, float intensity);
    void AddDirLightToFrame(Vec3 dir, Vec3 col, float intensity);

private:
    static const unsigned int MAX_POINT_LIGHTS = 4;

    //Shader objects
    Shader* shader; 
    Shader* debugLightShader;
    Shader* screenShader;
    Shader* dirShadowShader;

    //TEXTURES
    unsigned int currentDiffuseTexture;
    unsigned int currentNormalMapTexture;
    unsigned int LoadTexture(const char* path);
    std::unordered_map<const char*, unsigned int> textureToID;
    void AddToTextureMap(const char* path); //stores texture to map if its not already there

    //LIGHTING
    void SetAndSendAllLightsToFalse();
    unsigned int currentFramePointLightCount;
    struct PointLight
    {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        bool isActive;

        PointLight()
            : position(0.0f), color(1.0f), intensity(1.0f), isActive(false) {}

        PointLight(const glm::vec3& pos, const glm::vec3& col, float intensity, bool active)
            : position(pos), color(col), intensity(intensity), isActive(active) {}
    };
    PointLight pointLights[MAX_POINT_LIGHTS];
    void SetPointLightProperties(unsigned int index, Vec3 pos, Vec3 col, float intensity, bool active);
    void SendPointLightUniforms(unsigned int index);
    void InitializePointLights();

    struct DirLight
    {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        bool isActive;

        DirLight()
            : direction(0.5f, -1.0f, 0.5f), color(1.0f), intensity(1.0f), isActive(false) {}

        DirLight(const glm::vec3& dir, const glm::vec3& col, float intensity, bool active)
            : direction(dir), color(col), intensity(intensity), isActive(active) {}
    };
    DirLight dirLight;
    void SetDirLightProperties(Vec3 dir, Vec3 col, float intensity, bool active);
    void SendDirLightUniforms();
    void InitializeDirLight();

    //SHADOWS
    //directional
    unsigned int D_SHADOW_WIDTH = 2048, D_SHADOW_HEIGHT = 2048;
    void RenderDirShadowMap();

    //point
    unsigned int P_SHADOW_WIDTH = 1024, P_SHADOW_HEIGHT = 1024;
    void RenderPointShadowMap();
    std::vector<glm::mat4> shadowTransforms;
    
    //DRAWING
    std::vector<DrawCall*> drawCalls;

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
    unsigned int finalImageFBO, finalImageTextureRGB, finalImageRBO; 
    void SetupDirShadowMapFramebuffer();
    unsigned int dirShadowMapFBO, dirShadowMapTextureDepth;
    void SetupPointShadowMapFramebuffer();
    unsigned int pointShadowMapFBO, pointShadowMapTextureDepth; //cube map

};