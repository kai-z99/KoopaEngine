#pragma once

#include <algorithm>
#include "KoopaMath.h"

struct AABB
{
    Vec3 min;
    Vec3 max;

    void expand(const Vec3& point) 
    {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    void expand(const AABB& other) {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);
        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
    }

    AABB()
    {
        min = Vec3(std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max());

        max = Vec3(std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest());
    }
};

/*
struct PointLightGPU
{
    glm::vec4 positionRange;
    glm::vec4 colorIntensity;
    uint32_t  isActive;
    int32_t  shadowMapIndex;
    uint32_t  pad0, pad1;
};
*/

struct MeshData
{
    unsigned int VAO;
    unsigned int vertexCount = 0; //for normal draw
    unsigned int indexCount = 0; //for index draw
    AABB aabb;
};

struct Material
{
    //the id of diffuse material texture
    unsigned int diffuse;
    Vec3 baseColor;
    bool useDiffuseMap;
    bool hasAlpha;
    
    //id
    unsigned int normal;
    bool useNormalMap;

    //id
    unsigned int specular;
    float baseSpecular;
    bool useSpecularMap;
};

struct PBRMaterial
{
    unsigned int albedo;
    unsigned int normal;
    unsigned int metallic;
    unsigned int roughness;
    unsigned int ao;
    unsigned int height;
};

constexpr float PI = 3.14159265359f;

constexpr unsigned int SCREEN_WIDTH = 1920;
constexpr unsigned int SCREEN_HEIGHT = 1080;

//max amount of point lights allowed
constexpr unsigned int MAX_POINT_LIGHTS = 1024;
constexpr unsigned int MAX_LIGHTS_PER_TILE = 256;
constexpr unsigned int TILE_SIZE = 16;
constexpr unsigned int MAX_SHADOW_CASTING_POINT_LIGHTS = 4;

//camera
constexpr float DEFAULT_EXPOSURE = 0.5f;
constexpr float DEFAULT_NEAR = 0.1f;
constexpr float DEFAULT_FAR = 600.0f;

//Tone mapping methods
enum ToneFunction
{
	REINHARD = 0,
	EXPOSURE,
};

enum FogType
{
    LINEAR = 0,
    EXPONENTIAL = 1,
    EXPONENTIAL_SQUARED = 2
};

//directional light shadow frustum settings
constexpr float D_FRUSTUM_SIZE = 10.0f;
constexpr float D_NEAR_PLANE = 1.0f;
constexpr float D_FAR_PLANE = 10.5f;

//amount off passes of gaussian blur in bloom
constexpr unsigned int BLUR_PASSES = 5;

//sphere vertex generation
constexpr unsigned int SPHERE_X_SEGMENTS = 64;
constexpr unsigned int SPHERE_Y_SEGMENTS = 64;

//LOD
constexpr unsigned int MINIMUM_VERTEX_COUNT_FOR_LOD = 100;

constexpr bool FRUSTUM_CULLING = true;