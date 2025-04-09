#include "pch.h"
#include "DrawCall.h"
#include "Shader.h"
#include "Model.h"


DrawCall::DrawCall(MeshData meshData, Material material, const glm::mat4& model, GLenum primitive)
{
    this->model = nullptr; //if this stays null, we are not drawing a model.
    this->heightMapPath = nullptr;  //if this stays null, we are not drawing terrain.

    //general data
    this->VAO = meshData.VAO;
    this->vertexCount = meshData.vertexCount;
    this->aabb = meshData.aabb;
    this->modelMatrix = model;
    this->primitive = primitive;
    this->aabb = meshData.aabb;

    //general flags
    this->usingCulling = true;

    //Material properties
    this->material = material;
}

DrawCall::DrawCall(Model* m, const glm::mat4 model)
{
    this->heightMapPath = nullptr;  //if this stays null, we are not drawing terrain.
    this->model = m;
    this->modelMatrix = model;
    this->aabb = m->aabb;

    //general flags
    this->usingCulling = true;
}

void DrawCall::Render(Shader* shader)
{
    shader->use();

    if (usingCulling) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(this->modelMatrix));

    if (this->model != nullptr)
    {
        this->model->Draw(*shader);
    }
    else
    {
        glBindVertexArray(VAO);
        glDrawArrays(this->primitive, 0, this->vertexCount);
    }
    
    glEnable(GL_CULL_FACE);
    glBindVertexArray(0);
    
}

void DrawCall::BindMaterialUniforms(Shader* shader)
{
    shader->use();

    if (this->model != nullptr)
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingModel"), 1);
        return; //Model has its own version of the stuff below.
    }
    else
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingModel"), 0);
    }

    // Send base values. (these always exist)
    Vec3 baseColor = this->material.baseColor;
    glUniform3fv(glGetUniformLocation(shader->ID, "material.baseColor"), 1, glm::value_ptr(glm::vec3(baseColor.r, baseColor.g, baseColor.b)));
    glUniform1f(glGetUniformLocation(shader->ID, "material.baseSpecular"), this->material.baseSpecular);

    //Send usingX flags
    glUniform1i(glGetUniformLocation(shader->ID, "usingDiffuseMap"), this->material.useDiffuseMap);
    glUniform1i(glGetUniformLocation(shader->ID, "usingNormalMap"), this->material.useNormalMap);
    glUniform1i(glGetUniformLocation(shader->ID, "usingSpecularMap"), this->material.useSpecularMap);

    //TEXTURE0: DIFFUSEMAP------------------------------------------------------------------------
    if (this->material.useDiffuseMap)
    {
        glActiveTexture(GL_TEXTURE0); //in fs1: material.diffuse is 0
        glBindTexture(GL_TEXTURE_2D, this->material.diffuse); 
    }

    //TEXTURE1: NORMALMAP-------------------------------------------------------------------------
    if (this->material.useNormalMap)
    {
        glActiveTexture(GL_TEXTURE1); //in fs1: material.normal is 1
        glBindTexture(GL_TEXTURE_2D, this->material.normal);
    }

    //TEXTURE2: SPECULARMAP-----------------------------------------------------------------------
    if (this->material.useSpecularMap)
    {
        glActiveTexture(GL_TEXTURE2); //in fs1: material.specular is 2
        glBindTexture(GL_TEXTURE_2D, this->material.specular);
    }
}

void DrawCall::SetCulling(bool enabled)
{
    this->usingCulling = enabled;
}

const char* DrawCall::GetHeightMapPath()
{
    return this->heightMapPath;
}

void DrawCall::SetHeightMapPath(const char* path)
{
    assert(this->model == nullptr);
    this->heightMapPath = path;
}

AABB DrawCall::GetWorldAABB() const
{
    //Vec3 -> glm::vec3
    glm::vec3 min = glm::vec3(this->aabb.min.x, this->aabb.min.y, this->aabb.min.z);
    glm::vec3 max = glm::vec3(this->aabb.max.x, this->aabb.max.y, this->aabb.max.z);

    std::vector<glm::vec3> corners =
    {
        // Bottom face (y = min.y)
        {min.x, min.y, min.z},  // L-B-F: left, bottom, (-z)
        {max.x, min.y, min.z},  // R-B-F: right, bottom,  (-z)
        {min.x, min.y, max.z},  // L-B-B: left, bottom,  (+z)
        {max.x, min.y, max.z},  // R-B-B: right, bottom,  (+z)

        // Top face (y = max.y)
        {min.x, max.y, min.z},  // L-T-F: left, top,  (-z)
        {max.x, max.y, min.z},  // R-T-F: right, top,  (-z)
        {min.x, max.y, max.z},  // L-T-B: left, top,  (+z)
        {max.x, max.y, max.z}   // R-T-B: right, top,  (+z)
    };

    AABB worldAABB;

    for (const glm::vec3& corner : corners)
    {
        glm::vec4 worldCorner = this->modelMatrix * glm::vec4(corner, 1.0f);
        
        worldAABB.expand(Vec3(worldCorner.x, worldCorner.y, worldCorner.z));

    }

    return worldAABB;
}
