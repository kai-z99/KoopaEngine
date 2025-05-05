#pragma once

#include "KoopaMath.h"
#include "Definitions.h"
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <utility>

class Shader;
class ComputeShader;
class DrawCall;
class Camera;
class Model;
class ParticleEmitter;

class Renderer
{
public:
    //TEMP
    Camera* cam;
    std::vector<ParticleEmitter*> particleEmitters;
    ParticleEmitter* p;
    ParticleEmitter* p2;

    //Construction
	Renderer();
	~Renderer();
    
    //Begin/End frame
    void BeginRenderFrame();
    void EndRenderFrame();

    //Drawing Functions
    void ClearScreen(Vec4 col);
    void DrawTriangle(Vec3 pos, Vec4 rotation);
    void DrawCube(Vec3 pos, Vec3 size, Vec4 rotation);
    void DrawPlane(Vec3 pos, Vec2 size, Vec4 rotation);
    void DrawSphere(Vec3 pos, Vec3 size, Vec4 rotation);
    void DrawModel(const char* path, bool flipTexture, Vec3 pos, Vec3 size, Vec4 rotation);
    void DrawTerrain(const char* path, Vec3 pos, Vec3 size, Vec4 rotation);
    void CreateParticleEmitter(Vec3 pos, Vec3 size, Vec4 rotation);

    //Lighting
    void AddPointLightToFrame(Vec3 pos, Vec3 col, float range, float intensity, bool shadow);
    void AddDirLightToFrame(Vec3 dir, Vec3 col, float intensity, bool shadow);
    bool drawDebugLights;

    //uniform updates
    void SendCameraUniforms(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position);
    void SendOtherUniforms();

    //Material and Post processing settings
    void ResetMaterial();
    void SetCurrentDiffuse(const char* path);
    void SetCurrentBaseColor(Vec3 col);
    void SetCurrentNormal(const char* path);
    void SetCurrentSpecular(const char* path);
    void SetBaseSpecular(float spec);
    void SetSkybox(const std::vector<const char*>& faces);
    void SetExposure(float exposure);
    void SetFogType(FogType fog);
    void SetFogColor(Vec3 col);
    void SetExpFogDensity(float density);
    void SetLinearFogStart(float start);
    void SetAmbientLighting(float ambient);
    void SetBloomThreshold(float threshold);

private:
    //CONSTRUCTOR 
    void InitializeShaders();
    void InitializePointLights();
    void InitializeDirLight();
    void SetupVertexBuffers();
    void SetupFramebuffers();
    void SetupSSAOData();

    //RENDER PASSES
    void RenderShadowMaps();
    void RenderSSAO();
    void RenderMainScene();
    void DrawLightsDebug();
    void DrawSkybox();
    void BlurBrightScene();
    void DrawFinalQuad();

    //SHADER OBJECTS
    Shader* lightingShader; 
    Shader* debugLightShader;
    Shader* screenShader;
    Shader* brightShader; //takes in a hdr scene and extracts the bright parts.
    Shader* dirShadowShader;
    Shader* cascadeShadowShader;
    Shader* pointShadowShader;
    Shader* vsmPointBlurShader;
    Shader* skyShader;
    Shader* blurShader;
    Shader* terrainShader;
    Shader* geometryPassShader;
    Shader* ssaoShader;
    Shader* ssaoBlurShader;
    Shader* particleShader;
    ComputeShader* c;
    ComputeShader* particleUpdateComputeShader;

    //COMMAND BUFFER
    std::vector<DrawCall*> drawCalls;

    //MATERIAL
    Material currentMaterial;
    std::unordered_map<const char*, unsigned int> textureToID;
    void AddToTextureMap(const char* path); //stores texture to map if its not already there

    //SKYBOX
    unsigned int currentSkyboxTexture;
    bool usingSkybox;

    //FOG
    FogType fogType;
    glm::vec3 fogColor;
    float expFogDensity;
    float linearFogStart;

    //MODELS & TERRAIN
    std::unordered_map<const char*, Model*> pathToModel; //path to model : model*   
    std::unordered_map<const char*, std::pair<MeshData, unsigned int>> pathToTerrainMeshDataAndTextureID; //path : <Meshdata, texture>

    //LIGHTING DATA
    float ambientLighting;
    float bloomThreshold;
    void SetAndSendAllLightsToFalse();
    //point
    struct PointLight
    {
        glm::vec3 position;
        glm::vec3 color;
        float range;
        float intensity;
        bool isActive;
        bool castShadows;

        PointLight()
            : position(0.0f), color(1.0f), range(2.0f), intensity(1.0f), isActive(false), castShadows(false) {}

        PointLight(const glm::vec3& pos, const glm::vec3& col, float range, float intensity, bool active, bool shadow)
            : position(pos), color(col), range(range), intensity(intensity), isActive(active), castShadows(shadow) {}
    };
    PointLight pointLights[MAX_POINT_LIGHTS];
    unsigned int currentFramePointLightCount;
    void SendPointLightUniforms(Shader* shader, unsigned int index);
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
    
    //SHADOWS
    //cascade
    unsigned int CASCADE_SHADOW_WIDTH = 1024, CASCADE_SHADOW_HEIGHT = 1024;
    std::vector<float> cascadeLevels;
    std::vector<float> cascadeMultipliers;
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
    glm::mat4 CalculateLightSpaceCascadeMatrix(float near, float far, int index);
    std::vector<glm::mat4> GetCascadeMatrices();
    void RenderCascadedShadowMap();
    //point
    unsigned int P_SHADOW_WIDTH = 512, P_SHADOW_HEIGHT = 512;
    std::vector<glm::mat4> shadowTransforms;
    void RenderPointShadowMap(unsigned int index);
    void BlurPointShadowMap(unsigned int index);

    //FRUSTUM CULLING
    bool IsAABBVisible(const AABB& worldAABB, glm::vec4* frustumPlanes);
    void GetFrustumPlanes(const glm::mat4& vp, glm::vec4* frustumPlanes);
    glm::vec4 cameraFrustumPlanes[6];

    //SSAO
    std::vector<glm::vec3> ssaoKernel;
    std::vector<glm::vec3> ssaoNoise = {};
    unsigned int ssaoNoiseTexture;

    //MESH DATA
    MeshData triangleMeshData;
    MeshData cubeMeshData;
    MeshData sphereMeshData;
    MeshData planeMeshData;
    MeshData screenQuadMeshData;
    MeshData skyboxMeshData;
    unsigned int terrainVAO;

    //FRAMEBUFFERS/TEXTURES
    unsigned int hdrFBO, hdrMSAAFBO, hdrTextureRGBA, hdrMSAATextureRGBA;
    unsigned int twoPassBlurFBOs[2], twoPassBlurTexturesRGBA[2];
    unsigned int halfResBrightFBO, halfResBrightTextureRGBA;
    unsigned int dirShadowMapFBO, dirShadowMapTextureDepth;
    unsigned int cascadeShadowMapFBO, cascadeShadowMapTextureArrayDepth;
    unsigned int pointShadowMapFBO, pointShadowMapTextureArrayRG; 
    unsigned int vsmBlurFBO[2], vsmBlurTextureArrayRG[2];
    unsigned int gBufferFBO, gNormalTextureRGBA, gPositionTextureRGBA; 
    unsigned int ssaoFBO, ssaoBlurFBO, ssaoQuadTextureR, ssaoBlurTextureR;
    unsigned int T1;


    //MISC DATA
    Vec4 clearColor;
    bool msaa;
};