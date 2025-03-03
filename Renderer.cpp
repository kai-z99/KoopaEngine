#include "pch.h"
#include "Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "shaderSources.h"
#include "Shader.h"

#include <iostream>

Renderer::Renderer()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

	this->SetupBuffers();
    this->shader = new Shader(ShaderSources::vs1, ShaderSources::fs1);
    this->shader->use();

    this->debugLightShader = new Shader(ShaderSources::vs1, ShaderSources::fsLight);

    //temp
    this->currentDiffuseTexture = this->LoadTexture("../ShareLib/Resources/wood.png");

    glUniform1i(glGetUniformLocation(this->shader->ID, "currentTexture"), 0); //GL_TEXTIRE0

    //temp
    PointLight p0 = PointLight();
    p0.isActive = true;
    p0.color = { 1.0f, 1.0f, 1.0f };
    p0.position = { 1.0f, 4.0f, 0.0f };
    p0.intensity = 1.0f;

    PointLight p1 = PointLight();
    p1.isActive = true;
    p1.color = { 0.0f, 1.0f, 0.0f };
    p1.position = { -1.0f, 1.0f, 0.0f };
    p1.intensity = 1.0f;

    PointLight p2 = PointLight();
    p2.isActive = true;
    p2.color = { 0.0f, 0.0f, 1.0f };
    p2.position = { 1.0f, 3.0f, 2.0f };
    p2.intensity = 1.0f;

    PointLight p3 = PointLight();
    p3.isActive = true;
    p3.color = { 1.0f, 0.0f, 0.0f };
    p3.position = { 0.0f, -3.0f, 0.0f };
    p3.intensity = 1.0f;

    this->pointLights[0] = p0;
    this->pointLights[1] = p1;
    this->pointLights[2] = p2;
    this->pointLights[3] = p3;

    for (unsigned int i = 0; i < 4; i++)
    {
        std::string s = "pointLights[" + std::to_string(i) + "].position";
        const char* cs = s.c_str();
        glUniform3fv(glGetUniformLocation(this->shader->ID, cs), 1, glm::value_ptr(this->pointLights[i].position));

        s = "pointLights[" + std::to_string(i) + "].color";
        const char* csc = s.c_str();
        glUniform3fv(glGetUniformLocation(this->shader->ID, csc), 1, glm::value_ptr(this->pointLights[i].color));

        s = "pointLights[" + std::to_string(i) + "].isActive";
        const char* csa = s.c_str();
        glUniform1i(glGetUniformLocation(this->shader->ID, csa), this->pointLights[i].isActive);

        s = "pointLights[" + std::to_string(i) + "].intensity";
        const char* csi = s.c_str();
        glUniform1f(glGetUniformLocation(this->shader->ID, csi), this->pointLights[i].intensity);
    }    
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &triangleVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    delete this->shader;
}

void Renderer::ClearScreen(Vec4 col)
{
    glClearColor(col.r, col.g, col.b, col.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::SetCurrentDiffuse(const char* path)
{
    auto it = this->textureToID.find(path);

    if (it == this->textureToID.end())
    {
        unsigned int textureID = this->LoadTexture(path);
        this->textureToID[path] = textureID;
    }

    this->currentDiffuseTexture = this->textureToID[path];
}

void Renderer::DrawTriangle(Vec3 pos, Vec4 rotation)
{
    glDisable(GL_CULL_FACE); //cant cull flat things
    shader->use();
    glBindVertexArray(this->triangleVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->currentDiffuseTexture);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, pos.z));
    model = glm::rotate(model, glm::radians(rotation.w), glm::vec3(rotation.x, rotation.y, rotation.z));
    glUniformMatrix4fv(glGetUniformLocation(this->shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, 0, 3);
    glEnable(GL_CULL_FACE);
}

void Renderer::DrawCube(Vec3 pos, Vec3 size, Vec4 rotation)
{
    shader->use();
    glBindVertexArray(this->cubeVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->currentDiffuseTexture);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, pos.z));
    model = glm::rotate(model, glm::radians(rotation.w), glm::vec3(rotation.x, rotation.y, rotation.z));
    model = glm::scale(model, glm::vec3(size.x, size.y, size.z));
    glUniformMatrix4fv(glGetUniformLocation(this->shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::DrawPlane(Vec3 pos, Vec3 size, Vec4 rotation)
{
    glDisable(GL_CULL_FACE); //cant cull flat things
    shader->use();
    glBindVertexArray(this->planeVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->currentDiffuseTexture);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, pos.z));
    model = glm::rotate(model, glm::radians(rotation.w), glm::vec3(rotation.x, rotation.y, rotation.z));
    model = glm::scale(model, glm::vec3(size.x, size.y, size.z));
    glUniformMatrix4fv(glGetUniformLocation(this->shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_CULL_FACE);
}

void Renderer::DrawLightsDebug()
{
    this->debugLightShader->use();
    
    glBindVertexArray(cubeVAO);
    for (int i = 0; i < 4; i++)
    {
        PointLight curr = this->pointLights[i];
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(curr.position.x, curr.position.y, curr.position.z));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        glUniformMatrix4fv(glGetUniformLocation(this->debugLightShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(debugLightShader->ID, "lightColor"), 1, glm::value_ptr(curr.color));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
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

void Renderer::SetupBuffers()
{
	this->SetupTriangleBuffers();
	this->SetupCubeBuffers();
    this->SetupPlaneBuffers();
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
        // positions           // normals         // tex coords             // tangents
        -15.0f,  -0.5f,  15.0f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
         15.0f,  -0.5f,  15.0f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,    1.0f, 0.0f, 0.0f,
         15.0f,  -0.5f, -15.0f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,

         15.0f,  -0.5f, -15.0f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        -15.0f,  -0.5f, -15.0f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        -15.0f,  -0.5f,  15.0f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
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
