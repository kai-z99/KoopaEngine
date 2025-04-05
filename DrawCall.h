#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include "KoopaMath.h"

class Shader;
class Model;

class DrawCall
{
public:
	DrawCall(unsigned int VAO, unsigned int vertexCount, const glm::mat4& model, GLenum primitive = GL_TRIANGLES);
	DrawCall(Model* m, const glm::mat4 model);

	//Draw
	void Render(Shader* shader);

	//For "fs1" lighting shader
	void SendMaterialUniforms(Shader* shader);
	void SetNormalMapTexture(unsigned int id);
	void SetDiffuseMapTexture(unsigned int id);
	void SetSpecularIntensity(float shiny);
	void SetDiffuseColor(Vec3 col);

	//General flags
	void SetCulling(bool enabled);

	const char* GetHeightMapPath();
	void SetHeightMapPath(const char* path);

private:
	//General data
	unsigned int VAO;
	unsigned int vertexCount;
	glm::mat4 modelMatrix;
	GLenum primitive;

	//Material
	bool usingDiffuseMap;
	bool usingNormalMap;
	unsigned int diffuseMapTexture;
	Vec3 diffuseColor;
	unsigned int normalMapTexture;
	float specularIntensity;

	//for terrain
	const char* heightMapPath;

	//General flags
	bool usingCulling;

	//Drawing a predefined model
	Model* model;
};