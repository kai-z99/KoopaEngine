#include "pch.h"
#include "Setup.h"
#include <glad/glad.h>
#include <stb_image.h>

#include "Definitions.h"
#include <iostream>

namespace VertexBufferSetup
{
	unsigned int SetupTriangleBuffers()
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

        // Position attribute (location = 0 in the vertex shader)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Unbind (optional)
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

		return VAO;
	}

    unsigned int SetupCubeBuffers()
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

        return VAO;
    }

    unsigned int SetupSphereBuffers()
    {
        unsigned int VAO, VBO;

        std::vector<float> vertices;

        // Each vertex will have: 3 (position) + 3 (normal) + 2 (texCoord) + 3 (tangent) = 11 floats.
        // Loop over each quad on the sphere’s surface and compute two triangles per quad.
        for (unsigned int y = 0; y < SPHERE_Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x < SPHERE_X_SEGMENTS; ++x)
            {
                // Compute normalized texture coordinates for the current quad corners.
                float u0 = (float)x / SPHERE_X_SEGMENTS;
                float u1 = (float)(x + 1) / SPHERE_X_SEGMENTS;
                float v0 = (float)y / SPHERE_Y_SEGMENTS;
                float v1 = (float)(y + 1) / SPHERE_Y_SEGMENTS;

                // Compute positions for the four corners of the quad.
                float x0 = std::cos(u0 * 2.0f * PI) * std::sin(v0 * PI);
                float y0 = std::cos(v0 * PI);
                float z0 = std::sin(u0 * 2.0f * PI) * std::sin(v0 * PI);

                float x1 = std::cos(u1 * 2.0f * PI) * std::sin(v0 * PI);
                float y1 = std::cos(v0 * PI);
                float z1 = std::sin(u1 * 2.0f * PI) * std::sin(v0 * PI);

                float x2 = std::cos(u0 * 2.0f * PI) * std::sin(v1 * PI);
                float y2 = std::cos(v1 * PI);
                float z2 = std::sin(u0 * 2.0f * PI) * std::sin(v1 * PI);

                float x3 = std::cos(u1 * 2.0f * PI) * std::sin(v1 * PI);
                float y3 = std::cos(v1 * PI);
                float z3 = std::sin(u1 * 2.0f * PI) * std::sin(v1 * PI);

                // Lambda to add a vertex's data to our vector.
                auto addVertex = [&](float posX, float posY, float posZ, float u, float v) {
                    // Position
                    vertices.push_back(posX);
                    vertices.push_back(posY);
                    vertices.push_back(posZ);
                    // Normal (same as position for a unit sphere)
                    vertices.push_back(posX);
                    vertices.push_back(posY);
                    vertices.push_back(posZ);
                    // Texture coordinates
                    vertices.push_back(u);
                    vertices.push_back(v);
                    // Tangent: derivative with respect to u (ignoring the v-dependent factor)
                    float tanX = -std::sin(u * 2.0f * PI);
                    float tanY = 0.0f;
                    float tanZ = std::cos(u * 2.0f * PI);
                    // Normalize tangent
                    float length = std::sqrt(tanX * tanX + tanY * tanY + tanZ * tanZ);
                    if (length > 0.0f)
                    {
                        tanX /= length;
                        tanY /= length;
                        tanZ /= length;
                    }
                    vertices.push_back(tanX);
                    vertices.push_back(tanY);
                    vertices.push_back(tanZ);
                    };

                // Instead of v0, v2, v1 => do v0, v1, v2
                addVertex(x0, y0, z0, u0, v0); // v0
                addVertex(x1, y1, z1, u1, v0); // v1
                addVertex(x2, y2, z2, u0, v1); // v2

                // Instead of v1, v2, v3 => do v1, v3, v2
                addVertex(x1, y1, z1, u1, v0); // v1
                addVertex(x3, y3, z3, u1, v1); // v3
                addVertex(x2, y2, z2, u0, v1); // v2
            }
        }

        // Create and bind VAO and VBO, then upload vertex data.
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

        return VAO;
    }


    unsigned int SetupPlaneBuffers()
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

        return VAO;
    }

    unsigned int SetupScreenQuadBuffers()
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

        return VAO;

    }

    unsigned int SetupSkyboxBuffers()
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

        return VAO;
    }
}

namespace FramebufferSetup
{
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

    void SetupHalfResBrightFramebuffer(unsigned int& FBO, unsigned int texture)
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

    void SetupCascadedShadowMapFramebuffer(unsigned int& FBO, unsigned int& textureArray, unsigned int w, unsigned int h, unsigned int numCascades)
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
        //attach to fb
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

    void SetupPointShadowMapFramebuffer(unsigned int& FBO)
    {
        //create framebuffer
        glGenFramebuffers(1, &FBO);

        //Set buffers
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glDrawBuffer(GL_NONE); //No need for color buffer, just depth
        glReadBuffer(GL_NONE); //No need for color buffer

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //Note: texture is not attched to FB yet. They will be attached when the faces are actaully rendered.
        //the reason why its not like this for the dir shadow is because its not cube so theres only one possible thing
        //to render to when doing the shadow pass. For this we need to know which face to render to.
    }
}

namespace TextureSetup
{
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



