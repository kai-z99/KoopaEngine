//#include "pch.h"
#include "../include/Setup.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <iostream>
#include "../include/Definitions.h"


static inline AABB GetAABB(float* vertexData, unsigned int vertexCount, unsigned int stride)
{
    //Calcuate AABB;
    Vec3 minBounds = Vec3(std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max());

    Vec3 maxBounds = Vec3(std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest());

    for (unsigned int i = 0; i < vertexCount; i++)
    {
        unsigned int index = stride * i;
        Vec3 pos = Vec3(vertexData[index], vertexData[index + 1], vertexData[index + 2]);

        minBounds.x = std::min(pos.x, minBounds.x);
        minBounds.y = std::min(pos.y, minBounds.y);
        minBounds.z = std::min(pos.z, minBounds.z);

        maxBounds.x = std::max(pos.x, maxBounds.x);
        maxBounds.y = std::max(pos.y, maxBounds.y);
        maxBounds.z = std::max(pos.z, maxBounds.z);
    }

    AABB aabb;
    aabb.max = maxBounds;
    aabb.min = minBounds;

    return aabb;
}

namespace VertexBufferSetup
{
	MeshData SetupTriangleBuffers()
	{
        unsigned int VAO, VBO;

        float vertices[] = {
         0.0f,  0.5f, 0.0f,  // top
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f   // bottom left
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        // Bind and set vertex buffer(s) and configure vertex attributes
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // glVertexAttribPointer snapshots the current GL_ARRAY_BUFFER binding into the VAO�s attribute state.
        // Position attribute (location = 0 in the vertex shader)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind (optional)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        MeshData result;
        result.VAO = VAO;
        result.vertexCount = 3;
        result.aabb = GetAABB(vertices, result.vertexCount, 3);

        return result;
	}

    MeshData SetupCubeBuffers()
    {
        unsigned int VAO, VBO;

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

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
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

        MeshData result;
        result.vertexCount = 36;
        result.VAO = VAO;
        result.aabb = GetAABB(cubeVertices, result.vertexCount, 11);

        return result;
    }

    MeshData SetupSphereBuffers()
    {
        unsigned int VAO, VBO;

        std::vector<float> vertices;

        const float TILE_U = 2.0f;   
        const float TILE_V = 2.0f;   

        auto addVertex = [&](const glm::vec3& pos, float uAngle, float vAngle, float uTex, float vTex)
            {
                glm::vec3 N = glm::normalize(pos);

                float theta = uAngle * 2.0f * PI;
                float phi = vAngle * PI;
                glm::vec3 T = glm::normalize(glm::vec3(
                    -sin(theta) * sin(phi),
                    0.0f,
                    cos(theta) * sin(phi)
                ));

                float len2 = glm::dot(T, T);
                T = (len2 > 1e-8f)
                    ? glm::normalize(T)
                    : glm::vec3(1.0f, 0.0f, 0.0f);

                vertices.insert(vertices.end(),
                    {
                      pos.x, pos.y, pos.z,
                      N.x,   N.y,   N.z,
                      uTex,  vTex,
                      T.x,   T.y,   T.z
                    });
            };

        for (unsigned int y = 0; y < SPHERE_Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x < SPHERE_X_SEGMENTS; ++x)
            {

                float uAngle0 = (float)x / SPHERE_X_SEGMENTS;
                float uAngle1 = (float)(x + 1) / SPHERE_X_SEGMENTS;
                float vAngle0 = (float)y / SPHERE_Y_SEGMENTS;
                float vAngle1 = (float)(y + 1) / SPHERE_Y_SEGMENTS;
                    
                float uTex0 = uAngle0 * TILE_U;
                float uTex1 = uAngle1 * TILE_U;
                float vTex0 = vAngle0 * TILE_V;
                float vTex1 = vAngle1 * TILE_V;
    
                auto evalPos = [](float u, float v) {
                    float theta = u * 2.0f * PI;
                    float phi = v * PI;
                    return glm::vec3(cos(theta) * sin(phi),
                        cos(phi),
                        sin(theta) * sin(phi));
                    };

                glm::vec3 p0 = evalPos(uAngle0, vAngle0);
                glm::vec3 p1 = evalPos(uAngle1, vAngle0);
                glm::vec3 p2 = evalPos(uAngle0, vAngle1);
                glm::vec3 p3 = evalPos(uAngle1, vAngle1);


                

                addVertex(p0, uAngle0, vAngle0, uTex0, vTex0);
                addVertex(p1, uAngle1, vAngle0, uTex1, vTex0);
                addVertex(p2, uAngle0, vAngle1, uTex0, vTex1);

                addVertex(p1, uAngle1, vAngle0, uTex1, vTex0);
                addVertex(p3, uAngle1, vAngle1, uTex1, vTex1);
                addVertex(p2, uAngle0, vAngle1, uTex0, vTex1);
            }
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Set vertex attribute pointers:
        // Position attribute (location = 0): 3 floats.
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);

        // Normal attribute (location = 1): 3 floats.
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));

        // Texture coordinate attribute (location = 2): 2 floats.
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));

        // Tangent attribute (location = 3): 3 floats.
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));

        glBindVertexArray(0);

        MeshData result;
        result.VAO = VAO;
        result.vertexCount = (unsigned int)(vertices.size() / 11); //SPHERE_X_SEGMENTS * SPHERE_Y_SEGMENTS * 6
        result.aabb = GetAABB(&vertices[0], result.vertexCount, 11);

        return result;
    }

    MeshData SetupPlaneBuffers()
    {
        unsigned int VAO, VBO;

        float planeVertices[] = {
            // positions           // normals              // tex coords       // tangents
            -0.5f,    0.0f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
             0.5f,    0.0f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,    1.0f, 0.0f, 0.0f,
             0.5f,    0.0f,  -0.5f,   0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,

             0.5f,    0.0f,  -0.5f,   0.0f,  1.0f,  0.0f,    1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
            -0.5f,    0.0f,  -0.5f,   0.0f,  1.0f,  0.0f,    0.0f, 1.0f,    1.0f, 0.0f, 0.0f,
            -0.5f,    0.0f,   0.5f,   0.0f,  1.0f,  0.0f,    0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
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

        MeshData result;
        result.VAO = VAO;
        result.vertexCount = 6;
        result.aabb = GetAABB(planeVertices, result.vertexCount, 11);

        return result;
    }

    MeshData SetupScreenQuadBuffers()
    {
        unsigned int VAO, VBO;

        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); //pos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1); //tex
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);

        MeshData result;
        result.VAO = VAO;
        result.vertexCount = 6;
        result.aabb = GetAABB(quadVertices, result.vertexCount, 4); //dont really need this but for consitancy

        return result;

    }

    MeshData SetupSkyboxBuffers()
    {
        unsigned int VAO, VBO;

        float skyboxVertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        // skybox VAO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);

        MeshData result;
        result.VAO = VAO;
        result.vertexCount = 36;
        result.aabb = GetAABB(skyboxVertices, result.vertexCount, 3);

        return result;
    }
    
    std::pair<MeshData, unsigned int> SetupTerrainBuffers(const char* path)
    {
        // load and create a texture
        // -------------------------
        unsigned int heightMapTexture;

        glGenTextures(1, &heightMapTexture);
        glBindTexture(GL_TEXTURE_2D, heightMapTexture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // load image, create texture and generate mipmaps
        int width, height, nrChannels;
        // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
        unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format = GL_RGBA; // Assuming RGBA for now
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            // else format = GL_RGBA; // Default

            // Use the determined format
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "!!! OpenGL Error after glTexImage2D for heightmap: " << err << std::endl;
            }

            glGenerateMipmap(GL_TEXTURE_2D);
            err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "!!! OpenGL Error after glGenerateMipmap for heightmap: " << err << std::endl;
            }
        }
        else // stbi_load failed
        {
            std::cout << "Failed to load heightmap texture" << std::endl;
        }
        stbi_image_free(data); // Free data regardless of glTexImage2D success

        // set up vertex data (and buffer(s)) and configure vertex attributes
        // ------------------------------------------------------------------

        //vertex data for a flat plane with rez * rez patches and size: width * height
        std::vector<float> vertices;

        //rez * rez patches generated.
        unsigned rez = 20;
        for (unsigned i = 0; i <= rez - 1; i++)
        {
            for (unsigned j = 0; j <= rez - 1; j++)
            {
                //GENERATE PATCH
                //TL WORLD
                vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
                vertices.push_back(0.0f); // v.y
                vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
                vertices.push_back(i / (float)rez); // u
                vertices.push_back(j / (float)rez); // v

                //TR WORLD
                vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez); // v.x
                vertices.push_back(0.0f); // v.y
                vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
                vertices.push_back((i + 1) / (float)rez); // u
                vertices.push_back(j / (float)rez); // v

                //BL WORLD
                vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
                vertices.push_back(0.0f); // v.y
                vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez); // v.z
                vertices.push_back(i / (float)rez); // u
                vertices.push_back((j + 1) / (float)rez); // v

                //BR WORLD
                vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez); // v.x
                vertices.push_back(0.0f); // v.y
                vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez); // v.z
                vertices.push_back((i + 1) / (float)rez); // u
                vertices.push_back((j + 1) / (float)rez); // v
            }
        }

        // first, configure the cube's VAO (and terrainVBO)
        unsigned int VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // texCoord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
        glEnableVertexAttribArray(1);

        glPatchParameteri(GL_PATCH_VERTICES, 4);

        glBindVertexArray(0);

        MeshData result;
        result.VAO = VAO;
        result.vertexCount = rez * rez * 4;
        result.aabb = GetAABB(&vertices[0], result.vertexCount, 5);

        return {result, heightMapTexture};
    }
}

namespace FramebufferSetup
{
    /*
    void SetupHDRFramebuffer(unsigned int& FBO, unsigned int colorBuffers[2])
    {
        unsigned int RBO;
        //create and bind
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        //texture color buffer attachments (one for normal scene, one for only bright objects)
        glGenTextures(2, colorBuffers);

        for (unsigned int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL); //16f to hold greater than 1.0
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0); //attach to framebuffer
        }                                           //layout location here

        //renderbuffer attachment (depth/stencil)
        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT); //allocate memory 
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        //Need this for MRT
        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        //finish
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    */

    void SetupHDRFramebuffer(unsigned int& FBO, unsigned int& texture)
    {
        unsigned int RBO;
        //create and bind
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        //texture color buffer attachments (one for normal scene, one for only bright objects)
        glGenTextures(1, &texture);


        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL); //16f to hold greater than 1.0
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0); //attach to framebuffer

        //renderbuffer attachment (depth/stencil)
        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT); //allocate memory 
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        //finish
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SetupMSAAHDRFramebuffer(unsigned int& FBO, unsigned int& texture)
    {
        unsigned int RBO;
        //create and bind
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        //texture color buffer attachments (one for normal scene, one for only bright objects)
        glGenTextures(1, &texture);

        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, GL_TRUE); //16f to hold greater than 1.0
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D_MULTISAMPLE, texture, 0); //attach to framebuffer
        
        //renderbuffer attachment (depth/stencil)
        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT); //allocate memory 
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

        //finish
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SetupHalfResBrightFramebuffer(unsigned int& FBO, unsigned int& texture)
    {
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //attach
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Half-resolution framebuffer not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SetupTwoPassBlurFramebuffers(unsigned int FBOs[2], unsigned int colorBuffers[2])
    {
        //generate
        glGenFramebuffers(2, FBOs);
        glGenTextures(2, colorBuffers);
                  
        for (int i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, FBOs[i]);
            glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
            //set texture params
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // clamp to the edge as the blur filter would otherwise sample repeated texture values
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //eg 1.1 -> 1.0 , -22.2 -> 0.0
            //attach to fb
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffers[i], 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "Framebuffer not complete!" << std::endl;
        }
    }

    void SetupVSMTwoPassBlurFramebuffer(unsigned int& FBO, unsigned int& textureArray, unsigned int w, unsigned int h)
    {
        //generate
        glGenFramebuffers(1, &FBO);
        glGenTextures(1, &textureArray);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        /* no attachments yet � attached per-layer during blur passes */

        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textureArray);

        //set texture params
        glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RG32F, w, h, MAX_SHADOW_CASTING_POINT_LIGHTS * 6, 0, GL_RG, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    }
    
    //If FRAGMENT is far enough away, use mip level > 0.
    //If FRAGMENT is far enough away, use mip level > 0.
    //If FRAGMENT is far enough away, use mip level > 0.
    //If FRAGMENT is far enough away, use mip level > 0.
    //If FRAGMENT is far enough away, use mip level > 0.
    //If FRAGMENT is far enough away, use mip level > 0.


    void SetupDirShadowMapFramebuffer(unsigned int& FBO, unsigned int& texture, unsigned int w, unsigned int h)
    {
        //create framebuffer
        glGenFramebuffers(1, &FBO);

        //Create texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); //DONT LET THE TEXTURE MAP REPEAT
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); //DONT LET THE TEXTURE MAP REPEAT
        float borderCol[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderCol); // Instead, outside its range should just be 1.0f (furthest away, so no shadow)
        //NOTE: VIEWPORT HAS NO RELATION TO THIS, ITS FOR PROJECTION MATRIX FRUSTUM. (when frag is outside this frustum)

        //Attach texture
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
        glDrawBuffer(GL_NONE); //No need for color buffer
        glReadBuffer(GL_NONE); //No need for color buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SetupCascadedShadowMapFramebuffer(unsigned int& FBO, unsigned int& textureArray, unsigned int w, unsigned int h, int numCascades)
    {
        //create fb
        glGenFramebuffers(1, &FBO);

        //create tex
        glGenTextures(1, &textureArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray); //texture array
        //3d, depth value is the amount of textures              one partition: 2 cascades etc...
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, w, h, numCascades, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); 
        //params
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderCol[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderCol);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        //we dont need to attach the entire array to fb
        //glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureArray, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        /*
        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer cascade is not complete!";
        }
        */

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SetupPointShadowMapFramebuffer(unsigned int& FBO, unsigned int w, unsigned int h)
    {
        //create framebuffer
        glGenFramebuffers(1, &FBO);

        //Set buffers
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        unsigned int RBO;
        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        //Note: texture is not attched to FB yet. They will be attached when the faces are actaully rendered.
        //the reason why its not like this for the dir shadow is because its not cube so theres only one possible thing
        //to render to when doing the shadow pass. For this we need to know which face to render to.
    }

    void SetupSSAOFramebuffer(unsigned int& FBO, unsigned int& texture)
    {
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    static struct PointLightGPU
    {
        glm::vec4 positionRange;
        glm::vec4 colorIntensity;
        uint32_t  isActive;
        int32_t  shadowMapIndex;
        uint32_t  pad0, pad1;
    };

    void SetupTiledSSBOs(unsigned int& lightSSBO, unsigned int& countSSBO, unsigned int& indexSSBO)
    {

        uint32_t nx = (SCREEN_WIDTH + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t ny = (SCREEN_HEIGHT + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t nTiles = nx * ny;

        //TEMP
        glCreateBuffers(1, &lightSSBO);
        glCreateBuffers(1, &indexSSBO);
        glCreateBuffers(1, &countSSBO);

        //lights
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PointLightGPU) * MAX_POINT_LIGHTS, NULL, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lightSSBO); //binding = 1

        //indexSSBO
        size_t indexBufBytes = nTiles * MAX_LIGHTS_PER_TILE * sizeof(GLuint);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, indexBufBytes, nullptr, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indexSSBO);

        //count
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, countSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, nTiles * sizeof(GLuint), nullptr, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, countSSBO);

        static_assert(sizeof(PointLightGPU) % 16 == 0, "std430 alignment");

    }

    void SetupGBufferFramebuffer(unsigned int& FBO, unsigned int& gNormal, unsigned int& gPosition)
    {
        //fbo
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        //normal texture (view space) ATTACHMENT 0
        glGenTextures(1, &gNormal);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gNormal, 0); //attach

        // position texture (view space) ATTACHMENT 1
        glGenTextures(1, &gPosition);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gPosition, 0);

        //depth testing
        unsigned int rboDepth; //LOST
        glGenRenderbuffers(1, &rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREEN_WIDTH, SCREEN_HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

namespace TextureSetup
{
    void SetupPointShadowMapTextureArray(unsigned int& textureArray, unsigned int w, unsigned int h)
    {
        glGenTextures(1, &textureArray);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textureArray);

        glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RG32F, w, h, MAX_SHADOW_CASTING_POINT_LIGHTS * 6, 0, GL_RG, GL_FLOAT, NULL);

        //filtering
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //sampling
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    }

    void SetupSSAONoiseTexture(unsigned int& texture, const std::vector<glm::vec3>& noise)
    {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    unsigned int LoadTexture(char const* path)
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
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

    unsigned int LoadTextureCubeMap(const std::vector<const char*>& faces)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        int width, height, nrComponents;
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char* data = stbi_load(faces[i], &width, &height, &nrComponents, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return textureID;
    }
}

/*
    void SetupPointShadowMapTexture(unsigned int& texture, unsigned int w, unsigned int h)
    {
        //createa and bind the cube map
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

        //attach each face of the cubemap with a depth texture
        //can't use rbo since  we have to sample the depth in the shader.
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); //DONT LET THE TEXTURE MAP REPEAT
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //DONT LET THE TEXTURE MAP REPEAT
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); //DONT LET THE TEXTURE MAP REPEAT
    }
    */


