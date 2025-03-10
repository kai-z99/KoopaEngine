#include "pch.h"
#include "Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "shaderSources.h"
#include "Constants.h"
#include "Shader.h"

#include <iostream>

Renderer::Renderer()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    //Static setup
	this->SetupVertexBuffers();
    this->SetupFramebuffers();

    //Main lighting shader
    this->shader = new Shader(ShaderSources::vs1, ShaderSources::fs1);
    this->shader->use();
    this->currentDiffuseTexture = this->LoadTexture("../ShareLib/Resources/wood.png");
    this->currentNormalMapTexture = this->LoadTexture("../ShareLib/Resources/brickwall_normal.jpg");
    glUniform1i(glGetUniformLocation(this->shader->ID, "currentDiffuse"), 0); //GL_TEXTURE0
    glUniform1i(glGetUniformLocation(this->shader->ID, "currentNormalMap"), 1); //GL_TEXTURE1

    //Debug lighting shader
    this->debugLightShader = new Shader(ShaderSources::vs1, ShaderSources::fsLight);

    //Final quad shader
    this->screenShader = new Shader(ShaderSources::vsScreenQuad, ShaderSources::fsScreenQuad);
    this->screenShader->use();
    glUniform1i(glGetUniformLocation(this->screenShader->ID, "screenTexture"), 0); //GL_TEXTIRE0

    //Initialialize lighting
    this->InitializePointLights();
    this->InitializeDirLight();
    this->currentFramePointLightCount = 0;
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &triangleVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    delete this->shader;
    delete this->debugLightShader;
}

void Renderer::BeginRenderFrame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, this->finalImageFBO); //off screen render
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::EndRenderFrame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST); //will be drawing directly in front screen
    glClear(GL_COLOR_BUFFER_BIT);

    //Draw the final scene
    this->screenShader->use();
    glBindVertexArray(this->screenQuadVAO); //whole screen
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->finalImageTextureColorBuffer); // main scene framebuffer texture to GL_TEXTURE0
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    this->SetAndSendAllLightsToFalse(); //uniforms are sent here too.

    //Dont transfer SetCurrentDiffuse() to next frame.
    this->shader->use();
    glUniform1i(glGetUniformLocation(this->shader->ID, "usingNormalMap"), 0); //reset normal
    glUniform1i(glGetUniformLocation(this->shader->ID, "usingDiffuseMap"), 0); //reset diffuse
    this->SetCurrentColorDiffuse(noTexturePink);

} 

void Renderer::ClearScreen(Vec4 col)
{
    glClearColor(col.r, col.g, col.b, col.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::AddToTextureMap(const char* path)
{
    auto it = this->textureToID.find(path);

    if (it == this->textureToID.end())
    {
        unsigned int textureID = this->LoadTexture(path);
        this->textureToID[path] = textureID;
    }
}

void Renderer::SetCurrentDiffuse(const char* path)
{
    AddToTextureMap(path);
    this->currentDiffuseTexture = this->textureToID[path];
    glUniform1i(glGetUniformLocation(this->shader->ID, "usingDiffuseMap"), true);
}

void Renderer::SetCurrentColorDiffuse(Vec3 col)
{
    this->shader->use();
    glUniform3fv(glGetUniformLocation(this->shader->ID, "baseColor"), 1, glm::value_ptr(glm::vec3(col.r, col.g, col.b)));
    glUniform1i(glGetUniformLocation(this->shader->ID, "usingDiffuseMap"), false);
}

void Renderer::SetCurrentNormal(const char* path)
{
    AddToTextureMap(path);
    this->currentNormalMapTexture = this->textureToID[path];
    glUniform1i(glGetUniformLocation(this->shader->ID, "usingNormalMap"), true);
}

void Renderer::BindDiffuseAndNormalTextures() const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->currentDiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, this->currentNormalMapTexture);
}

void Renderer::DrawMesh(Shader& shader, unsigned int VAO, unsigned int vertexCount, const glm::mat4& modelMatrix)
{
    shader.use();
    glBindVertexArray(VAO);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);

    glBindVertexArray(0);
}

static glm::mat4 CreateModelMatrix(const Vec3& pos, const Vec4& rotation, const Vec3& scale)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, pos.z));
    model = glm::rotate(model, glm::radians(rotation.w), glm::vec3(rotation.x, rotation.y, rotation.z));
    model = glm::scale(model, glm::vec3(scale.x, scale.y, scale.z));
    return model;
}

void Renderer::DrawTriangle(Vec3 pos, Vec4 rotation)
{
    glDisable(GL_CULL_FACE); //cant cull flat things

    this->BindDiffuseAndNormalTextures();
    glm::mat4 model = CreateModelMatrix(pos, rotation, {1,1,1});
    this->DrawMesh(*this->shader, this->triangleVAO, 3, model);

    glEnable(GL_CULL_FACE);
}

void Renderer::DrawCube(Vec3 pos, Vec3 size, Vec4 rotation)
{
    this->BindDiffuseAndNormalTextures();
    glm::mat4 model = CreateModelMatrix(pos, rotation, size);
    this->DrawMesh(*this->shader, this->cubeVAO, 36, model);
}

void Renderer::DrawPlane(Vec3 pos, Vec2 size, Vec4 rotation)
{
    glDisable(GL_CULL_FACE); //cant cull flat things

    this->BindDiffuseAndNormalTextures();
    glm::mat4 model = CreateModelMatrix(pos, rotation, Vec3(size.x, 1.0f, size.y));
    this->DrawMesh(*this->shader, this->planeVAO, 6, model);

    glEnable(GL_CULL_FACE);
}

void Renderer::DrawLightsDebug()
{
    this->debugLightShader->use();
    
    glBindVertexArray(cubeVAO);
    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        PointLight curr = this->pointLights[i];
        if (!curr.isActive) continue;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(curr.position.x, curr.position.y, curr.position.z));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        glUniformMatrix4fv(glGetUniformLocation(this->debugLightShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(debugLightShader->ID, "lightColor"), 1, glm::value_ptr(curr.color));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

void Renderer::AddPointLightToFrame(Vec3 pos, Vec3 col, float intensity)
{
    if (this->currentFramePointLightCount + 1 <= MAX_POINT_LIGHTS)
    {
        this->SetPointLightProperties(this->currentFramePointLightCount, pos, col, intensity, true);
        SendPointLightUniforms(this->currentFramePointLightCount);
        this->currentFramePointLightCount++;
    }
    else
    {
        std::cout << "ERROR: Max pointlights exceeded\n";
    }
}

void Renderer::AddDirLightToFrame(Vec3 dir, Vec3 col, float intensity)
{
    this->SetDirLightProperties(dir, col, intensity, true);
    this->SendDirLightUniforms();
}

void Renderer::SetAndSendAllLightsToFalse()
{
    for (int i = 0; i < 4; i++)
    {
        this->pointLights[i].isActive = false;
        this->SendPointLightUniforms(i);
    }
    this->currentFramePointLightCount = 0;

    this->dirLight.isActive = false;
    this->SendDirLightUniforms();
}

void Renderer::SetPointLightProperties(unsigned int index, Vec3 pos, Vec3 col, float intensity, bool active)
{
    this->pointLights[index].position = {pos.x, pos.y, pos.z};
    this->pointLights[index].color = { col.r, col.g, col.b };
    this->pointLights[index].intensity = intensity;
    this->pointLights[index].isActive = active;
}

void Renderer::SendPointLightUniforms(unsigned int index)
{
    this->shader->use();

    std::string s = "pointLights[" + std::to_string(index) + "].position";
    const char* cs = s.c_str();
    glUniform3fv(glGetUniformLocation(this->shader->ID, cs), 1, glm::value_ptr(this->pointLights[index].position));

    s = "pointLights[" + std::to_string(index) + "].color";
    const char* csc = s.c_str();
    glUniform3fv(glGetUniformLocation(this->shader->ID, csc), 1, glm::value_ptr(this->pointLights[index].color));

    s = "pointLights[" + std::to_string(index) + "].isActive";
    const char* csa = s.c_str();
    glUniform1i(glGetUniformLocation(this->shader->ID, csa), this->pointLights[index].isActive);

    s = "pointLights[" + std::to_string(index) + "].intensity";
    const char* csi = s.c_str();
    glUniform1f(glGetUniformLocation(this->shader->ID, csi), this->pointLights[index].intensity);
}

void Renderer::SetDirLightProperties(Vec3 dir, Vec3 col, float intensity, bool active)
{
    this->dirLight.direction = { dir.x, dir.y, dir.z };
    this->dirLight.color = {col.r, col.g, col.b};
    this->dirLight.intensity = intensity;
    this->dirLight.isActive = active;
}

void Renderer::SendDirLightUniforms()
{
    this->shader->use();

    glUniform3fv(glGetUniformLocation(this->shader->ID, "dirLight.direction"), 1, glm::value_ptr(dirLight.direction));
    glUniform3fv(glGetUniformLocation(this->shader->ID, "dirLight.color"), 1, glm::value_ptr(dirLight.color));
    glUniform1f(glGetUniformLocation(this->shader->ID, "dirLight.intensity"), dirLight.intensity);
    glUniform1i(glGetUniformLocation(this->shader->ID, "dirLight.isActive"), dirLight.isActive);
}

void Renderer::InitializePointLights()
{
    PointLight p0 = PointLight();
    PointLight p1 = PointLight();
    PointLight p2 = PointLight();
    PointLight p3 = PointLight();

    this->pointLights[0] = p0;
    this->pointLights[1] = p1;
    this->pointLights[2] = p2;
    this->pointLights[3] = p3;

    for (unsigned int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        this->SendPointLightUniforms(i);
    }
}

void Renderer::InitializeDirLight()
{
    DirLight d = DirLight();
    this->SendDirLightUniforms();
}

void Renderer::SetCameraMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position)
{
    shader->use();
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(shader->ID, "viewPos"), 1, glm::value_ptr(position));

    debugLightShader->use();
    glUniformMatrix4fv(glGetUniformLocation(debugLightShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(debugLightShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));    
}

void Renderer::SetupFramebuffers()
{
    this->SetupFinalImageFramebuffer();
}

void Renderer::SetupFinalImageFramebuffer()
{
    //create and bind
    glGenFramebuffers(1, &this->finalImageFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, this->finalImageFBO);

    //texture color buffer attachment
    glGenTextures(1, &this->finalImageTextureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, this->finalImageTextureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); //create a new RGB texture with NULL as its data
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->finalImageTextureColorBuffer, 0);

    //renderbuffer attachment (depth/stencil)
    glGenRenderbuffers(1, &this->finalImageRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, this->finalImageRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600); //allocate memory 
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->finalImageRBO);

    //finish
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

unsigned int Renderer::LoadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        GLenum internalFormat;
        if (nrComponents == 1)
        {
            format = GL_RED;
            internalFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = GL_RGB;
            format = GL_RGB;
        }

        else if (nrComponents == 4)
        {
            internalFormat = GL_RGBA;
            format = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        if (format == GL_RGBA)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void Renderer::SetupTriangleBuffers()
{
    float vertices[] = {
     0.0f,  0.5f, 0.0f,  // top
     0.5f, -0.5f, 0.0f,  // bottom right
    -0.5f, -0.5f, 0.0f   // bottom left
    };

    glGenVertexArrays(1, &this->triangleVAO);
    glGenBuffers(1, &this->triangleVBO);

    // Bind and set vertex buffer(s) and configure vertex attributes
    glBindVertexArray(this->triangleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute (location = 0 in the vertex shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind (optional)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::SetupVertexBuffers()
{
    this->SetupTriangleBuffers();
    this->SetupCubeBuffers();
    this->SetupPlaneBuffers();
    this->SetupScreenQuadBuffers();
}

void Renderer::SetupCubeBuffers()
{
    float cubeVertices[] = {
        // Front face (z = +0.5)
        // positions              // normals         // tex coords     // tangents
        -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,    1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,

         0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,

        // Back face (z = -0.5)
        -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f,    1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,

         0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f,    1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,

        // Left face (x = -0.5)
        -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    0.0f, 0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,    0.0f, 0.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,    0.0f, 0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,    0.0f, 0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,    0.0f, 0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    0.0f, 0.0f,  1.0f,

        // Right face (x = +0.5) 
        0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    0.0f, 0.0f,  -1.0f,
        0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f,    0.0f, 0.0f,  -1.0f,
        0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,    0.0f, 0.0f,  -1.0f,

        0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,    0.0f, 0.0f,  -1.0f,
        0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f,    0.0f, 0.0f,  -1.0f,
        0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,    0.0f, 0.0f,  -1.0f,

        // Top face (y = +0.5)
        -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,    1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,

         0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,

        // Bottom face (y = -0.5)
        -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f,    1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,

         0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,    1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);

    // normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));

    // texCoord attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));

    // tangent attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::SetupPlaneBuffers()
{
    float planeVertices[] = {
        // positions           // normals              // tex coords       // tangents
        -0.5f,    0.0f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
         0.5f,    0.0f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,    1.0f, 0.0f, 0.0f,
         0.5f,    0.0f,  -0.5f,   0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,

         0.5f,    0.0f,  -0.5f,   0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        -0.5f,    0.0f,  -0.5f,   0.0f,  1.0f,  0.0f,    0.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        -0.5f,    0.0f,   0.5f,   0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
    };

    glGenVertexArrays(1, &this->planeVAO);
    glGenBuffers(1, &this->planeVBO);

    glBindVertexArray(this->planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::SetupScreenQuadBuffers()
{
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &this->screenQuadVAO);
    glGenBuffers(1, &this->screenQuadVBO);
    glBindVertexArray(this->screenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->screenQuadVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); //pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); //tex
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}
