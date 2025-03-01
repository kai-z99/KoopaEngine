#pragma once

#include "KoopaMath.h"
#include <glm/gtc/matrix_transform.hpp>

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

    void SetCameraMatrices(const glm::mat4& view, const glm::mat4& projection);

private:
    unsigned int triangleVAO, triangleVBO, planeVBO;
    unsigned int cubeVAO, cubeVBO, planeVAO;
    Shader* shader; 


    unsigned int currentDiffuseTexture;
    unsigned int LoadTexture(char const* path);

    void SetupBuffers();
    void SetupTriangleBuffers();
    void SetupCubeBuffers();
    void SetupPlaneBuffers();

};