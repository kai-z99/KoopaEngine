#include "pch.h"
#include "DrawCall.h"
#include "Shader.h"
#include "Constants.h"

DrawCall::DrawCall(unsigned int VAO, unsigned int vertexCount, const glm::mat4& model)
{
    this->VAO = VAO;
    this->vertexCount = vertexCount;
    this->model = model;

    this->usingDiffuseMap = false;
    this->usingNormalMap = false;
    this->diffuseMapTexture = -1;
    this->normalMapTexture = -1;

    this->diffuseColor = noTexturePink; //missingTexture color
}

void DrawCall::Render(Shader* shader)
{
    shader->use();
    
    glBindVertexArray(VAO);
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(this->model));
    glDrawArrays(GL_TRIANGLES, 0, this->vertexCount);

    glBindVertexArray(0);
}

void DrawCall::SendDiffuseAndNormalProperties(Shader* shader)
{
    shader->use();

    if (this->usingDiffuseMap)
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingDiffuseMap"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->diffuseMapTexture); //in fs: currentDiffuseMap = 0
    }
    else
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingDiffuseMap"), 0);
        glUniform3fv(glGetUniformLocation(shader->ID, "baseColor"), 1, glm::value_ptr(glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b)));
    }

    if (this->usingNormalMap)
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingNormalMap"), 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->normalMapTexture); //in fs: currentNormalMap = 1
    }
    else
    {
        glUniform1i(glGetUniformLocation(shader->ID, "usingNormalMap"), 0);
    }
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
