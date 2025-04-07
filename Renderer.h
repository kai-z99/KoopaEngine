#pragma once

#include "KoopaMath.h"
#include "Definitions.h"
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <utility>

class Shader;
class DrawCall;
class Camera;
class Model;

class Renderer
{
public:
    //TEMP
    Camera* cam;

    //Construction
	Renderer();
	~Renderer();
    
    //Begin/End frame
    void BeginRenderFrame();
    void EndRenderFrame();

    //Geometry drawing functions
    void SendCameraUniforms(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position);
    void ClearScreen(Vec4 col);
    void DrawTriangle(Vec3 pos, Vec4 rotation);
    void DrawCube(Vec3 pos, Vec3 size, Vec4 rotation);
    void DrawPlane(Vec3 pos, Vec2 size, Vec4 rotation);
    void DrawSphere(Vec3 pos, Vec3 size, Vec4 rotation);
    void DrawModel(const char* path, bool flipTexture, Vec3 pos, Vec3 size, Vec4 rotation);
    void DrawTerrain(const char* path, Vec3 pos, Vec3 size, Vec4 rotation);

    //Modification functions
    void SetCurrentDiffuse(const char* path, float specularIntensity);
    void SetCurrentColorDiffuse(Vec3 col, float specularIntensity);
    void SetCurrentNormal(const char* path);
    void SetSkybox(const std::vector<const char*>& faces);
    void SetExposure(float exposure);

	//Light functions
    void AddPointLightToFrame(Vec3 pos,Vec3 col, float intensity, bool shadow);
    void AddDirLightToFrame(Vec3 dir, Vec3 col, float intensity, bool shadow);
    bool drawDebugLights;

private:
    //Shader objects
    Shader* lightingShader; 
    Shader* debugLightShader;
    Shader* screenShader;
    Shader* dirShadowShader;
    Shader* cascadeShadowShader;
    Shader* pointShadowShader;
    Shader* skyShader;
    Shader* blurShader;
    Shader* terrainShader;

    //TEXTURES
    unsigned int currentDiffuseTexture;
    unsigned int currentNormalMapTexture;
    std::unordered_map<const char*, unsigned int> textureToID;
    void AddToTextureMap(const char* path); //stores texture to map if its not already there
    unsigned int currentSkyboxTexture;
    bool usingSkybox;
    void DrawSkybox();

    //MODEL
    std::unordered_map<const char*, Model*> pathToModel; //path to model : model*   

    //LIGHTING
    void SetAndSendAllLightsToFalse();
    //point
    unsigned int currentFramePointLightCount;
    struct PointLight
    {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        bool isActive;
        bool castShadows;

        PointLight()
            : position(0.0f), color(1.0f), intensity(1.0f), isActive(false), castShadows(false) {}

        PointLight(const glm::vec3& pos, const glm::vec3& col, float intensity, bool active, bool shadow)
            : position(pos), color(col), intensity(intensity), isActive(active), castShadows(shadow) {}
    };
    PointLight pointLights[MAX_POINT_LIGHTS];
    void SendPointLightUniforms(Shader* shader, unsigned int index);
    void InitializePointLights();
    float SHADOW_PROJECTION_FAR = 25.0f, SHADOW_PROJECTION_NEAR = 0.1f;
    //directional
    struct DirLight
    {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        bool isActive;
        bool castShadows;

        DirLight()
            : direction(0.5f, -1.0f, 0.5f), color(1.0f), intensity(1.0f), isActive(false), castShadows(false) {}

        DirLight(const glm::vec3& dir, const glm::vec3& col, float intensity, bool active, bool shadow)
            : direction(dir), color(col), intensity(intensity), isActive(active), castShadows(shadow) {}
    };
    DirLight dirLight;
    void SendDirLightUniforms();
    void InitializeDirLight();
    //debug
    void DrawLightsDebug();

    //SHADOWS
    //directional
    unsigned int D_SHADOW_WIDTH = 1024, D_SHADOW_HEIGHT = 1024;
    void RenderDirShadowMap();
    //cascade
    unsigned int CASCADE_SHADOW_WIDTH = 1024, CASCADE_SHADOW_HEIGHT = 1024;
    std::vector<float> cascadeLevels;
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
    glm::mat4 CalculateLightSpaceCascadeMatrix(float near, float far);
    std::vector<glm::mat4> GetCascadeMatrices();
    void RenderCascadedShadowMap();
    //point
    unsigned int P_SHADOW_WIDTH = 1024, P_SHADOW_HEIGHT = 1024;
    void RenderPointShadowMap(unsigned int index);
    std::vector<glm::mat4> shadowTransforms;
    
    //DRAWING
    std::vector<DrawCall*> drawCalls;
    void DrawFinalQuad();
    void BlurBrightScene();
    Vec4 clearColor;

    //VERTEX BUFFER/ARRAY
    void SetupVertexBuffers();
    unsigned int triangleVAO;
    MeshData cubeMeshData;
    MeshData sphereMeshData;
    MeshData planeMeshData;
    unsigned int screenQuadVAO;
    unsigned int skyboxVAO;
    unsigned int terrainVAO;
    std::unordered_map<const char*, std::pair<unsigned int, unsigned int>> pathToTerrainVAOandTexture; //path : <VAO, texture>

    //FRAMEBUFFERS
    void SetupFramebuffers();
    unsigned int hdrFBO, hdrColorBuffers[2]; //0: hdrTextureRGBA 1: hdrTextureBrightRGBA
    unsigned int twoPassBlurFBOs[2], twoPassBlurTexturesRGBA[2];
    unsigned int halfResBrightFBO, halfResBrightTextureRGBA;
    unsigned int dirShadowMapFBO, dirShadowMapTextureDepth;
    unsigned int cascadeShadowMapFBO, cascadeShadowMapTextureArrayDepth;
    unsigned int pointShadowMapFBO, pointShadowMapTextureArrayDepth; 
};