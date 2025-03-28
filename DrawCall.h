#pragma once

#include <glm/gtc/type_ptr.hpp>
#include "KoopaMath.h"

class Shader;
class Model;

class DrawCall
{
public:
	DrawCall(unsigned int VAO, unsigned int vertexCount, const glm::mat4& model);
	DrawCall(Model* m, const glm::mat4 model);

	//Draw
	void Render(Shader* shader);

	//For "fs1" lighting shader
	void BindTextureProperties(Shader* shader);
	void SetNormalMapTexture(unsigned int id);
	void SetDiffuseMapTexture(unsigned int id);
	void SetDiffuseColor(Vec3 col);

	//General flags
	void SetCulling(bool enabled);

private:
	//General data
	unsigned int VAO;
	unsigned int vertexCount;
	glm::mat4 modelMatrix;

	//For "fs1" lighting shader
	bool usingDiffuseMap;
	bool usingNormalMap;
	unsigned int diffuseMapTexture;
	Vec3 diffuseColor;
	unsigned int normalMapTexture;

	//General flags
	bool usingCulling;

	//Drawing a predefined model?
	Model* model;
};