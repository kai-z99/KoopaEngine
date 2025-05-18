#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include "KoopaMath.h"
#include "Definitions.h"

#include <optional>

class Shader;
class Model;

class DrawCall
{
public:
	DrawCall(MeshData meshData, Material material, const glm::mat4& model, GLenum primitive = GL_TRIANGLES);
	DrawCall(MeshData meshData, PBRMaterial pbrmaterial, const glm::mat4& model, GLenum primitive = GL_TRIANGLES);

	//Draw
	void Render(Shader* shader, bool tempDontCull = false);
	void RenderLOD(Shader* shader, bool tempDontCull = false);

	//For "fs1" lighting shader
	void BindMaterialUniforms(Shader* shader);
	void BindPBRMaterialUniforms(Shader* shader);

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

	std::optional<MeshData> lodMeshData;

	//Material
	Material material;
	PBRMaterial pbrmaterial;

	//for terrain
	const char* heightMapPath;

	//General flags
	bool usingCulling;
};