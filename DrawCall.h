#pragma once

#include <glm/gtc/type_ptr.hpp>
#include "KoopaMath.h"

class Shader;

class DrawCall
{
public:
	DrawCall(unsigned int VAO, unsigned int vertexCount, const glm::mat4& model);

	void Render(Shader* shader);
	void SendDiffuseAndNormalProperties(Shader* shader);

	void SetNormalMapTexture(unsigned int id);
	void SetDiffuseMapTexture(unsigned int id);
	void SetDiffuseColor(Vec3 col);

private:
	unsigned int VAO;
	unsigned int vertexCount;
	glm::mat4 model;

	bool usingDiffuseMap;
	bool usingNormalMap;

	unsigned int diffuseMapTexture;
	Vec3 diffuseColor;
	unsigned int normalMapTexture;
};