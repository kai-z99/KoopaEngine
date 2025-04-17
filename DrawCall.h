#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include "KoopaMath.h"
#include "Definitions.h"

class Shader;
class Model;

class DrawCall
{
public:
	DrawCall(MeshData meshData, Material material, const glm::mat4& model, GLenum primitive = GL_TRIANGLES);

	//Draw
	void Render(Shader* shader, bool tempDontCull = false);

	//For "fs1" lighting shader
	void BindMaterialUniforms(Shader* shader);

	void SetLODMesh(MeshData meshData);

	//General flags
	void SetCulling(bool enabled);

	const char* GetHeightMapPath();
	void SetHeightMapPath(const char* path);

	//frustum culling
	AABB GetWorldAABB() const;

private:
	//Mesh Data
	MeshData meshData;
	glm::mat4 modelMatrix;
	GLenum primitive;

	//Material
	Material material;

	//for terrain
	const char* heightMapPath;

	//General flags
	bool usingCulling;
};