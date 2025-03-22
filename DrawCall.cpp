#include "pch.h"
#include "DrawCall.h"
#include "Shader.h"
#include "Constants.h"

DrawCall::DrawCall(unsigned int VAO, unsigned int vertexCount, const glm::mat4& model)
{
    //general data
    this->VAO = VAO;
    this->vertexCount = vertexCount;
    this->model = model;

    //general flags
    this->usingCulling = true;

    //"fs1" lighting shader flags
    this->usingDiffuseMap = false;
    this->usingNormalMap = false;
    this->diffuseMapTexture = -1;
    this->normalMapTexture = -1;
    this->diffuseColor = noTexturePink; //missingTexture color
}

void DrawCall::Render(Shader* shader)
{
    shader->use();
    
    if (usingCulling) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    glBindVertexArray(VAO);
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(this->model));
    glDrawArrays(GL_TRIANGLES, 0, this->vertexCount);

    glEnable(GL_CULL_FACE);
    glBindVertexArray(0);
}

void DrawCall::BindTextureProperties(Shader* shader)
{
    shader->use();

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

    //TEXTURE2: DIRECTIONAL SHADOWMAP--------------------------------------------------------------
    //Done in renderer.

    //TEXTURE3: POINT SHADOWMAP
    //Done in renderer.
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

void DrawCall::SetDiffuseColor(Vec3 col)
{
    this->diffuseColor = col;
    this->usingDiffuseMap = false;
}

void DrawCall::SetCulling(bool enabled)
{
    this->usingCulling = enabled;
}
