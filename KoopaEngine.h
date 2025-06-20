#pragma once

#include <vector>

#include "include/KoopaMath.h"
#include "include/Definitions.h"

class GLFWwindow;
class Shader;
class Camera;
class Renderer;

class KoopaEngine {
public:
    KoopaEngine();        
    ~KoopaEngine();
            
    bool shouldCloseWindow() const;

    void BeginFrame();
    void EndFrame();
    float GetCurrentFrame();

    void ResetMaterial();
    void SetCurrentDiffuseTexture(const char* path);
    void SetCurrentDiffuseTexture(Vec3 col);
    void SetCurrentNormalTexture(const char* path);
    void SetCurrentSpecularTexture(const char* path);
    void SetCurrentBaseSpecular(float specular);
    void SetCurrentPBRMaterial(const char* albedo, const char* normal, const char* height, const char* metallic, const char* roughness, const char* ao);
    void SetFogColor(Vec3 col);
    void SetFogType(FogType type);
    void SetExpFogDensity(float density);
    void SetLinearFogStart(float start);
    void SetAmbientLighting(float ambient);
    void SetBloomThreshold(float threshold);

    void SetSkybox(const std::vector<const char*>& faces);

    void ClearScreen(Vec4 color);

    void DrawTriangle
    (
        Vec3 pos = { 0.0f, 0.0f, 0.0f }, 
        Vec4 rotation = {0.0f, 1.0f, 0.0f, 0.0f}
    );

    void DrawCube
    (
        Vec3 pos = { 0.0f, 0.0f, 0.0f }, 
        Vec3 size = {1.0f, 1.0f, 1.0f}, 
        Vec4 rotation = { 0.0f, 1.0f, 0.0f, 0.0f }
    );

    void DrawPlane
    (
        Vec3 pos = { 0,0,0 }, 
        Vec2 size = { 1,1 }, 
        Vec4 rotation = { 0,1,0,0 }
    );

    void DrawSphere
    (
        Vec3 pos = { 0,0,0 },
        Vec3 size = { 1,1,1 },
        Vec4 rotation = { 0,1,0,0 }
    );

    void DrawModel
    (
        const char* path,
        bool flipTexture = false,
        Vec3 pos = {0,0,0},
        Vec3 size = {1,1,1},
        Vec4 rotation = {0,1,0,0}
    );

    void DrawTerrain
    (
        const char* path,
        Vec3 pos = { 0,0,0 },
        Vec3 size = { 1,1,1 },
        Vec4 rotation = { 0,1,0,0 }
    );

    void DrawPointLight
    (
        Vec3 pos = {0,0,0},
        Vec3 col = {1,1,1},
        float range = 1.0f,
        float intensity = 1.0f,
        bool shadows = false
    );

    void DrawDirLight
    (
        Vec3 dir = { 1.0f, -1.0f, 1.0f },
        Vec3 col = { 1.0f, 1.0f, 1.0f }, 
        float intesity = 1.0f,
        bool shadows = false
    );

    void CreateParticleEmitter
    (
        double duration,
        unsigned int particleCount,
        Vec3 pos = { 0.0f, 0.0f, 0.0f },
        Vec3 scale = { 1.0f, 1.0f, 1.0f },
        Vec4 rotation = {0.0f, 1.0f, 0.0f, 0.0f}
    );

    void SetDrawLightsDebug(bool on);
    void SetCameraExposure(float exposure);
    void SetCameraSpeed(float speed);
    
private:
    void DrawFinalQuad();

    GLFWwindow* window;
    Renderer* renderer;
    Camera* camera;

    float currentFrame = 0;
    bool firstMouse = true;
    float lastX = (float)SCREEN_WIDTH / 2.0f;
    float lastY = (float)SCREEN_HEIGHT / 2.0f;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    void processInput();

    // Static callback wrappers
    static void framebuffer_size_callback_static(GLFWwindow* window, int width, int height);
    static void mouse_callback_static(GLFWwindow* window, double xposIn, double yposIn);
    static void scroll_callback_static(GLFWwindow* window, double xoffset, double yoffset);
};