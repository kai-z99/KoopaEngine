#include "../include/DrawCall.h"
#include "../include/Shader.h"
#include "../include/Model.h"

DrawCall::DrawCall(MeshData meshData, Material material, const glm::mat4& model, GLenum primitive)
{
    this->heightMapPath = nullptr;  //if this stays null, we are not drawing terrain.

    //general data
    this->meshData = meshData;

    this->modelMatrix = model;
    this->primitive = primitive;
    
    //general flags
    this->usingCulling = true;

    //Material properties
    this->material = material;
}

DrawCall::DrawCall(MeshData meshData, PBRMaterial pbrmaterial, const glm::mat4& model, GLenum primitive)
{
    this->heightMapPath = nullptr;  //if this stays null, we are not drawing terrain.

    //general data
    this->meshData = meshData;

    this->modelMatrix = model;
    this->primitive = primitive;

    //general flags
    this->usingCulling = true;

    //Material properties
    this->pbrmaterial = pbrmaterial;

}

void DrawCall::Render(Shader* shader, bool tempDontCull)
{
    shader->use();

    //cull?
    if (usingCulling && !tempDontCull) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);

    //send model
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(this->modelMatrix));

    //vind vao and draw
    glBindVertexArray(this->meshData.VAO);
    if (this->meshData.indexCount != 0)
    {
        //we are drawing with an EBO
        glDrawElements(this->primitive, this->meshData.indexCount, GL_UNSIGNED_INT, 0);
    }
    else
    {
        glDrawArrays(this->primitive, 0, this->meshData.vertexCount);
    }
    
    glEnable(GL_CULL_FACE);
    glBindVertexArray(0);
}

void DrawCall::RenderLOD(Shader* shader, bool tempDontCull)
{
    if (this->lodMeshData.has_value())
    {
        shader->use();

        //cull?
        if (usingCulling && !tempDontCull) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);

        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(this->modelMatrix));

        glBindVertexArray(this->lodMeshData->VAO);

        if (this->lodMeshData->indexCount != 0)
        {
            //we are drawing with an EBO
            glDrawElements(this->primitive, this->lodMeshData->indexCount, GL_UNSIGNED_INT, 0);
        }
        else
        {
            //usually this is not a possible case for LOD
            glDrawArrays(this->primitive, 0, this->lodMeshData->vertexCount);
        }


        glEnable(GL_CULL_FACE);
        glBindVertexArray(0);
    }
    else
    {
        //std::cout << "CANNOT RENDER LOD: NO LOD MESHDATA\n";
        this->Render(shader, tempDontCull);
    }
}

void DrawCall::BindMaterialUniforms(Shader* shader)
{
    shader->use();

    // Send base values. (these always exist)
    Vec3 baseColor = this->material.baseColor;
    glUniform3fv(glGetUniformLocation(shader->ID, "material.baseColor"), 1, glm::value_ptr(glm::vec3(baseColor.r, baseColor.g, baseColor.b)));
    glUniform1f(glGetUniformLocation(shader->ID, "material.baseSpecular"), this->material.baseSpecular);

    //Send usingX flags
    glUniform1i(glGetUniformLocation(shader->ID, "usingDiffuseMap"), this->material.useDiffuseMap);
    glUniform1i(glGetUniformLocation(shader->ID, "usingNormalMap"), this->material.useNormalMap);
    glUniform1i(glGetUniformLocation(shader->ID, "usingSpecularMap"), this->material.useSpecularMap);
    glUniform1i(glGetUniformLocation(shader->ID, "hasAlpha"), this->material.hasAlpha);

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

void DrawCall::BindPBRMaterialUniforms(Shader* shader)
{
    shader->use();

    glActiveTexture(GL_TEXTURE0); //pbrmaterial.albedo
    glBindTexture(GL_TEXTURE_2D, this->pbrmaterial.albedo);

    glActiveTexture(GL_TEXTURE1); 
    glBindTexture(GL_TEXTURE_2D, this->pbrmaterial.normal);

    glActiveTexture(GL_TEXTURE2); 
    glBindTexture(GL_TEXTURE_2D, this->pbrmaterial.metallic);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, this->pbrmaterial.roughness);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, this->pbrmaterial.ao);

    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, this->pbrmaterial.height);
}

void DrawCall::SetLODMesh(MeshData meshData)
{
    this->lodMeshData = std::move(meshData);
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
    this->heightMapPath = path;
}

AABB DrawCall::GetWorldAABB() const
{
    AABB aabb = this->meshData.aabb;

    //Vec3 -> glm::vec3
    glm::vec3 min = glm::vec3(aabb.min.x, aabb.min.y, aabb.min.z);
    glm::vec3 max = glm::vec3(aabb.max.x, aabb.max.y, aabb.max.z);

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

/*
GLint boundEBO = 0;
glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &boundEBO);

if (boundEBO != 0) //draw with glDrawElements
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boundEBO);

    GLint bytesize = 0;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytesize);

    unsigned int indicesSize = bytesize / sizeof(GLint);

    glDrawElements(this->primitive, indicesSize, GL_UNSIGNED_INT, 0);
}
*/