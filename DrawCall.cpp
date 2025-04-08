#include "pch.h"
#include "DrawCall.h"
#include "Shader.h"
#include "Constants.h"
#include "Model.h"


DrawCall::DrawCall(MeshData meshData, const glm::mat4& model, GLenum primitive)
{
    this->model = nullptr; //if this stays null, we are not drawing a model.
    this->heightMapPath = nullptr;  //if this stays null, we are not drawing terrain.

    //general data
    this->VAO = meshData.VAO;
    this->vertexCount = meshData.vertexCount;
    this->aabb = meshData.aabb;
    this->modelMatrix = model;
    this->primitive = primitive;

    //general flags
    this->usingCulling = true;

    //Material properties
    this->usingDiffuseMap = false;
    this->usingNormalMap = false;
    this->diffuseMapTexture = -1;
    this->normalMapTexture = -1;
    this->diffuseColor = noTexturePink; //missingTexture color
    this->specularIntensity = 1.0f;     //default depculat intensity if setdiffuse is never called
}

DrawCall::DrawCall(Model* m, const glm::mat4 model)
{
    this->heightMapPath = nullptr;  //if this stays null, we are not drawing terrain.
    this->model = m;
    this->modelMatrix = model;

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



void DrawCall::SendUniqueUniforms(Shader* shader)
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

    //TEXTURE0: DIFFUSEMAP------------------------------------------------------------------------
    glActiveTexture(GL_TEXTURE0);
    if (this->usingDiffuseMap)
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingDiffuseMap"), 1);
        glBindTexture(GL_TEXTURE_2D, this->diffuseMapTexture); //in fs: currentDiffuseMap = 0
    }
    else
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingDiffuseMap"), 0);
        glUniform3fv(glGetUniformLocation(shader->ID, "baseColor"), 1, glm::value_ptr(glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b)));
    }

    //TEXTURE1: NORMALMAP-------------------------------------------------------------------------
    glActiveTexture(GL_TEXTURE1);
    if (this->usingNormalMap)
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingNormalMap"), 1);
        glBindTexture(GL_TEXTURE_2D, this->normalMapTexture); //in fs: currentNormalMap = 1
    }
    else
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingNormalMap"), 0);
    }

    glUniform1f(glGetUniformLocation(shader->ID, "specularIntensity"), this->specularIntensity);
}

void DrawCall::SetNormalMapTexture(unsigned int id)
{
    this->normalMapTexture = id;
    this->usingNormalMap = true;
}

void DrawCall::SetDiffuseMapTexture(unsigned int id)
{
    this->diffuseMapTexture = id;
    this->usingDiffuseMap = true;
}

void DrawCall::SetSpecularIntensity(float shiny)
{
    this->specularIntensity = shiny;
}

void DrawCall::SetDiffuseColor(Vec3 col)
{
    this->diffuseColor = col;
    this->usingDiffuseMap = false;
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
